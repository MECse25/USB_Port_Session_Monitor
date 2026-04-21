/*
 * usb_led_monitor.c
 *
 * USB Device Session Monitor + Esrijan DDK v2.1 PGM LED Blinker
 *
 * The DDK board (VID:16c0 PID:05dc, V-USB firmware) accepts USB control
 * transfers to turn its PGM LED on/off:
 *
 *   bmRequestType = 0x40  (vendor | host-to-device | device)
 *   bRequest      = 1     (SET_LED)
 *   wValue        = 1     LED ON  /  0 = LED OFF
 *   wIndex        = 0, data = NULL, length = 0
 *
 * Blink pattern: N blinks per cycle, N = number of active USB devices.
 *
 * Compile:  make
 * Load:     sudo insmod usb_led_monitor.ko
 * Log:      cat /proc/usb_monitor
 * Unload:   sudo rmmod usb_led_monitor
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <linux/timekeeping.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/notifier.h>
#include <linux/timer.h>
#include <linux/version.h>
#include <linux/rtc.h>
#include <linux/atomic.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 7, 0)
#define timer_delete_sync(t) del_timer_sync(t)
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Student");
MODULE_DESCRIPTION("USB Session Monitor + DDK v2.1 PGM LED via USB control transfer");

/* ── DDK identity ─────────────────────────────────────────────── */
#define DDK_VENDOR_ID       0x16c0
#define DDK_PRODUCT_ID      0x05dc
#define DDK_CMD_SET_LED     1       /* bRequest                    */
#define DDK_LED_ON          1       /* wValue to turn LED on       */
#define DDK_LED_OFF         0       /* wValue to turn LED off      */
#define DDK_REQ_TYPE_OUT    0x40    /* vendor | host-to-device     */

/* ── session tracking ─────────────────────────────────────────── */
#define MAX_DEVICES  50
#define PROC_FILE    "usb_monitor"
#define NAME_LEN     64

struct usb_session {
    struct usb_device *udev;
    u16      vendor_id;
    u16      product_id;
    char     device_name[NAME_LEN];
    time64_t insert_time;
    time64_t remove_time;
    time64_t duration;
    int      active;
};

static struct usb_session sessions[MAX_DEVICES];
static int session_count = 0;
static DEFINE_SPINLOCK(session_lock);
static atomic_t active_count = ATOMIC_INIT(0);

/* ── DDK handle ───────────────────────────────────────────────── */
static struct usb_device *ddk_udev = NULL;
static DEFINE_MUTEX(ddk_lock);

/* ── workqueue for USB control messages ───────────────────────── */
struct led_work {
    struct work_struct work;
    int value;
};

static struct workqueue_struct *led_wq;

static void led_work_fn(struct work_struct *w)
{
    struct led_work *lw = container_of(w, struct led_work, work);
    struct usb_device *udev;
    int ret;

    mutex_lock(&ddk_lock);
    udev = ddk_udev;
    if (udev)
        usb_get_dev(udev);
    mutex_unlock(&ddk_lock);

    if (!udev) {
        kfree(lw);
        return;
    }

    ret = usb_control_msg(udev,
                          usb_sndctrlpipe(udev, 0),
                          DDK_CMD_SET_LED,
                          DDK_REQ_TYPE_OUT,
                          lw->value,
                          0,
                          NULL,
                          0,
                          1000);

    if (ret < 0 && ret != -ENODEV)
        printk(KERN_WARNING "USB Monitor: LED ctrl failed: %d\n", ret);

    usb_put_dev(udev);
    kfree(lw);
}

static void ddk_led_set(int val)
{
    struct led_work *lw;

    if (!led_wq)
        return;

    lw = kmalloc(sizeof(*lw), GFP_ATOMIC);
    if (!lw)
        return;

    INIT_WORK(&lw->work, led_work_fn);
    lw->value = val;
    queue_work(led_wq, &lw->work);
}

/* ── blink timer ──────────────────────────────────────────────── */
static struct timer_list blink_timer;
static int blink_phase  = 0;
static int blink_target = 0;

#define BLINK_ON_MS   150
#define BLINK_OFF_MS  150
#define CYCLE_GAP_MS  800

static void blink_callback(struct timer_list *t)
{
    int n = atomic_read(&active_count);

    if (n <= 0) {
        ddk_led_set(DDK_LED_OFF);
        blink_phase  = 0;
        blink_target = 0;
        mod_timer(&blink_timer, jiffies + msecs_to_jiffies(CYCLE_GAP_MS));
        return;
    }

    if (blink_phase == 0)
        blink_target = n;

    if (blink_phase < 2 * blink_target) {
        if (blink_phase % 2 == 0) {
            ddk_led_set(DDK_LED_ON);
            mod_timer(&blink_timer, jiffies + msecs_to_jiffies(BLINK_ON_MS));
        } else {
            ddk_led_set(DDK_LED_OFF);
            mod_timer(&blink_timer, jiffies + msecs_to_jiffies(BLINK_OFF_MS));
        }
        blink_phase++;
    } else {
        ddk_led_set(DDK_LED_OFF);
        blink_phase = 0;
        mod_timer(&blink_timer, jiffies + msecs_to_jiffies(CYCLE_GAP_MS));
    }
}

/* ── helpers ──────────────────────────────────────────────────── */
static void fmt_time(time64_t unix_sec, char *buf, size_t len)
{
    struct rtc_time tm;
    rtc_time64_to_tm(unix_sec, &tm);
    snprintf(buf, len, "%04d-%02d-%02d %02d:%02d:%02d UTC",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec);
}

static void get_device_name(struct usb_device *udev, char *buf, size_t len)
{
    char mfr[NAME_LEN] = {0}, prd[NAME_LEN] = {0};
    int  ml = 0, pl = 0;

    if (udev->descriptor.iManufacturer)
        ml = usb_string(udev, udev->descriptor.iManufacturer, mfr, sizeof(mfr));
    if (udev->descriptor.iProduct)
        pl = usb_string(udev, udev->descriptor.iProduct, prd, sizeof(prd));

    if (ml > 0 && pl > 0)      snprintf(buf, len, "%s %s", mfr, prd);
    else if (pl > 0)            snprintf(buf, len, "%s", prd);
    else if (ml > 0)            snprintf(buf, len, "%s", mfr);
    else                        snprintf(buf, len, "Unknown VID:%04x PID:%04x",
                                         le16_to_cpu(udev->descriptor.idVendor),
                                         le16_to_cpu(udev->descriptor.idProduct));
}

/* ── duration timer ───────────────────────────────────────────── */
static struct timer_list duration_timer;

static void update_duration(struct timer_list *t)
{
    struct timespec64 ts;
    unsigned long flags;
    int i;

    ktime_get_real_ts64(&ts);
    spin_lock_irqsave(&session_lock, flags);
    for (i = 0; i < session_count; i++)
        if (sessions[i].active)
            sessions[i].duration = ts.tv_sec - sessions[i].insert_time;
    spin_unlock_irqrestore(&session_lock, flags);
    mod_timer(&duration_timer, jiffies + HZ);
}

/* ── USB notifier ─────────────────────────────────────────────── */
static int usb_event(struct notifier_block *self,
                     unsigned long action, void *data)
{
    struct usb_device *udev = (struct usb_device *)data;
    struct timespec64  ts;
    unsigned long      flags;
    u16 vid, pid;
    int i;

    if (!udev)
        return NOTIFY_OK;

    vid = le16_to_cpu(udev->descriptor.idVendor);
    pid = le16_to_cpu(udev->descriptor.idProduct);

    /* DDK board — update handle, don't log as a session */
    if (vid == DDK_VENDOR_ID && pid == DDK_PRODUCT_ID) {
        if (action == USB_DEVICE_ADD) {
            mutex_lock(&ddk_lock);
            ddk_udev = udev;
            mutex_unlock(&ddk_lock);
            printk(KERN_INFO "USB Monitor: DDK board connected — LED active\n");
        } else if (action == USB_DEVICE_REMOVE) {
            mutex_lock(&ddk_lock);
            ddk_udev = NULL;
            mutex_unlock(&ddk_lock);
            printk(KERN_INFO "USB Monitor: DDK board disconnected\n");
        }
        return NOTIFY_OK;
    }

    /* All other devices */
    ktime_get_real_ts64(&ts);

    if (action == USB_DEVICE_ADD) {
        char name[NAME_LEN] = {0};
        get_device_name(udev, name, sizeof(name));

        spin_lock_irqsave(&session_lock, flags);
        if (session_count < MAX_DEVICES) {
            int idx = session_count++;
            sessions[idx].udev        = udev;
            sessions[idx].vendor_id   = vid;
            sessions[idx].product_id  = pid;
            strscpy(sessions[idx].device_name, name, NAME_LEN);
            sessions[idx].insert_time = ts.tv_sec;
            sessions[idx].remove_time = 0;
            sessions[idx].duration    = 0;
            sessions[idx].active      = 1;
            printk(KERN_INFO "USB Monitor: [CONNECTED] %s (%04x:%04x)\n",
                   name, vid, pid);
        }
        spin_unlock_irqrestore(&session_lock, flags);

        atomic_inc(&active_count);
        printk(KERN_INFO "USB Monitor: Active=%d → LED blinks %d time(s)\n",
               atomic_read(&active_count), atomic_read(&active_count));

    } else if (action == USB_DEVICE_REMOVE) {
        spin_lock_irqsave(&session_lock, flags);
        for (i = 0; i < session_count; i++) {
            if (sessions[i].active && sessions[i].udev == udev) {
                sessions[i].remove_time = ts.tv_sec;
                sessions[i].duration    = ts.tv_sec - sessions[i].insert_time;
                sessions[i].active      = 0;
                printk(KERN_INFO "USB Monitor: [REMOVED] %s — %lld sec\n",
                       sessions[i].device_name, sessions[i].duration);
                break;
            }
        }
        spin_unlock_irqrestore(&session_lock, flags);

        if (atomic_read(&active_count) > 0)
            atomic_dec(&active_count);
        printk(KERN_INFO "USB Monitor: Active=%d → LED blinks %d time(s)\n",
               atomic_read(&active_count), atomic_read(&active_count));
    }

    return NOTIFY_OK;
}

static struct notifier_block usb_nb = { .notifier_call = usb_event };

/* ── /proc ────────────────────────────────────────────────────── */
static int usb_monitor_show(struct seq_file *m, void *v)
{
    unsigned long flags;
    int i;

    seq_printf(m, "\n=== USB Device Session Monitor + DDK v2.1 PGM LED ===\n\n");

    mutex_lock(&ddk_lock);
    seq_printf(m, "DDK board    : %s\n",
               ddk_udev ? "CONNECTED — LED control active"
                        : "NOT FOUND  — plug in DDK board");
    mutex_unlock(&ddk_lock);

    seq_printf(m, "Active devs  : %d  (PGM LED blinks this many times)\n\n",
               atomic_read(&active_count));

    spin_lock_irqsave(&session_lock, flags);
    if (session_count == 0)
        seq_printf(m, "  No sessions yet. Plug in a USB device.\n\n");

    for (i = 0; i < session_count; i++) {
        char ins[40], rem[40];
        fmt_time(sessions[i].insert_time, ins, sizeof(ins));
        if (!sessions[i].active && sessions[i].remove_time)
            fmt_time(sessions[i].remove_time, rem, sizeof(rem));
        else
            strscpy(rem, "Still connected", sizeof(rem));

        seq_printf(m, "Session #%d\n",           i + 1);
        seq_printf(m, "  Device   : %s\n",        sessions[i].device_name);
        seq_printf(m, "  VID:PID  : %04x:%04x\n", sessions[i].vendor_id,
                                                   sessions[i].product_id);
        seq_printf(m, "  Inserted : %s\n",         ins);
        seq_printf(m, "  Removed  : %s\n",         rem);
        seq_printf(m, "  Duration : %lld sec\n",   sessions[i].duration);
        seq_printf(m, "  Status   : %s\n\n",
                   sessions[i].active ? "ACTIVE" : "DISCONNECTED");
    }
    spin_unlock_irqrestore(&session_lock, flags);
    seq_printf(m, "Total sessions: %d\n", session_count);
    return 0;
}

static int usb_monitor_open(struct inode *inode, struct file *file)
{
    return single_open(file, usb_monitor_show, NULL);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops usb_fops = {
    .proc_open    = usb_monitor_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};
#else
static const struct file_operations usb_fops = {
    .open    = usb_monitor_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};
#endif

/* ── scan already-connected devices at load ───────────────────── */
static int scan_existing(struct usb_device *udev, void *unused)
{
    u16 vid = le16_to_cpu(udev->descriptor.idVendor);
    u16 pid = le16_to_cpu(udev->descriptor.idProduct);

    if (vid == DDK_VENDOR_ID && pid == DDK_PRODUCT_ID) {
        mutex_lock(&ddk_lock);
        ddk_udev = udev;
        mutex_unlock(&ddk_lock);
        printk(KERN_INFO "USB Monitor: DDK board already connected\n");
    }
    return 0;
}

/* ── init ─────────────────────────────────────────────────────── */
static int __init usb_led_monitor_init(void)
{
    printk(KERN_INFO "USB LED Monitor: Loading...\n");

    led_wq = alloc_ordered_workqueue("ddk_led_wq", 0);
    if (!led_wq)
        return -ENOMEM;

    if (!proc_create(PROC_FILE, 0444, NULL, &usb_fops)) {
        printk(KERN_ERR "USB LED Monitor: cannot create /proc/%s\n", PROC_FILE);
        destroy_workqueue(led_wq);
        return -ENOMEM;
    }

    usb_for_each_dev(NULL, scan_existing);
    usb_register_notify(&usb_nb);

    timer_setup(&duration_timer, update_duration, 0);
    mod_timer(&duration_timer, jiffies + HZ);

    timer_setup(&blink_timer, blink_callback, 0);
    mod_timer(&blink_timer, jiffies + msecs_to_jiffies(500));

    printk(KERN_INFO "USB LED Monitor: Ready. cat /proc/%s\n", PROC_FILE);
    return 0;
}

/* ── exit ─────────────────────────────────────────────────────── */
static void __exit usb_led_monitor_exit(void)
{
    timer_delete_sync(&blink_timer);
    timer_delete_sync(&duration_timer);

    if (led_wq) {
        flush_workqueue(led_wq);
        ddk_led_set(DDK_LED_OFF);
        flush_workqueue(led_wq);
        destroy_workqueue(led_wq);
        led_wq = NULL;
    }

    usb_unregister_notify(&usb_nb);
    remove_proc_entry(PROC_FILE, NULL);
    printk(KERN_INFO "USB LED Monitor: Unloaded cleanly.\n");
}

module_init(usb_led_monitor_init);
module_exit(usb_led_monitor_exit);
