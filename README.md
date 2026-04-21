# USB Port Session Monitor using Linux Kernel Module

**Device Drivers — Mini-Project | Manipal School of Information Sciences, Manipal**

---

## Team Members

| Name | 
|------|
| Phanikrishna N M |
| Chirag Poojary | 
| Tejas Gowda | 

---

## What This Does

This Linux Kernel Module combines two features:

1. **USB session tracking** — logs every USB device plug/unplug with timestamps, VID:PID, device name, and live duration. Readable via `cat /proc/usb_monitor`.
2. **PGM LED blinker** — the DDK v2.1's onboard PGM LED blinks **N times per cycle** where N = number of currently connected USB devices.

### Blink Pattern

| Active USB Devices | PGM LED Behaviour |
|--------------------|-------------------|
| 0 | LED stays OFF |
| 1 | Blinks once per cycle |
| 2 | Blinks twice per cycle |
| N | Blinks N times |

Each blink = **150 ms ON → 150 ms OFF**. An **800 ms pause** separates each group.

---

## Five Required Kernel Components

| # | Component | API Used |
|---|-----------|----------|
| 1 | `/proc` entry | `proc_create()`, `seq_file` |
| 2 | Kernel timer | `timer_setup()`, `mod_timer()` |
| 3 | Work queue | `alloc_ordered_workqueue()`, `queue_work()` |
| 4 | Atomic operations | `atomic_t`, `atomic_inc()`, `atomic_dec()` |
| 5 | USB subsystem | `usb_register_notify()`, `usb_control_msg()` |

---

## Hardware Platform

| Component | Details |
|-----------|---------|
| Board | eSrijan DDK v2.1 |
| Microcontroller | LPC3250 (ARM9) |
| Firmware | LDDK v2.2 |
| USB VID:PID | 16C0:05DC |
| LED Controlled | PGM (Program Running) LED |
| Host System | Ubuntu 24.04, Kernel 6.x |

---

## Files

```
usb_led_monitor/
├── usb_led_monitor.c   ← kernel module source (all 5 components)
├── Makefile            ← build script
├── test_usb_led.sh     ← automated test script
└── README.md           ← this file
```

---

## How to Build and Run

### 1. Install kernel headers
```bash
sudo apt install linux-headers-$(uname -r)
```

### 2. Compile
```bash
make clean && make
```

### 3. Load the module
```bash
sudo insmod usb_led_monitor.ko
```

### 4. Check it loaded
```bash
lsmod | grep usb_led_monitor
dmesg | tail -8
```

Expected output:
```
USB LED Monitor: Loading...
USB LED Monitor: DDK board already connected
USB LED Monitor: Ready. cat /proc/usb_monitor
```

### 5. Read the session log
```bash
cat /proc/usb_monitor
```

### 6. Run the automated test script
```bash
chmod +x test_usb_led.sh
./test_usb_led.sh
```

### 7. Unload
```bash
sudo rmmod usb_led_monitor
```

---

## PGM LED — How It Works

The DDK board (VID:16C0 PID:05DC) receives USB vendor control transfers to toggle the PGM LED:

```
bmRequestType = 0x40   (vendor | host-to-device | device)
bRequest      = 1      (DDK_CMD_SET_LED)
wValue        = 1      LED ON  /  0 = LED OFF
wIndex        = 0, data = NULL, length = 0, timeout = 1000 ms
```

LED commands are sent from an **ordered workqueue** (`ddk_led_wq`) rather than directly from the kernel timer, because `usb_control_msg()` can sleep and must not be called from atomic/interrupt context.

---

## How the Blink Logic Works

```
active_count = 2  →  blink group = [ON OFF  ON OFF]  then 800ms gap  repeat

Phase 0: LED ON   (blink 1 start)   → 150ms
Phase 1: LED OFF  (blink 1 end)     → 150ms
Phase 2: LED ON   (blink 2 start)   → 150ms
Phase 3: LED OFF  (blink 2 end)     → 800ms gap → phase resets to 0
```

`blink_callback()` is a `timer_list` function that re-arms itself every call. `atomic_read(&active_count)` provides the current device count safely from softirq context without locks.

---

## Kernel Version Compatibility

| Issue | Old API | Fix |
|-------|---------|-----|
| Timer teardown | `del_timer_sync()` | `timer_delete_sync()` — renamed in 6.7; `#define` shim added |
| `/proc` file ops | `file_operations` (< 5.6) | `proc_ops` for kernel ≥ 5.6, guarded by `LINUX_VERSION_CODE` |

---

## Expected `/proc/usb_monitor` Output

```
=== USB Device Session Monitor + DDK v2.1 PGM LED ===

DDK board    : CONNECTED — LED control active
Active devs  : 2  (PGM LED blinks this many times)

Session #1
  Device   : SanDisk Ultra
  VID:PID  : 0781:5591
  Inserted : 2026-04-21 10:14:03 UTC
  Removed  : Still connected
  Duration : 87 sec
  Status   : ACTIVE

Session #2
  Device   : Logitech USB Receiver
  VID:PID  : 046d:c52b
  Inserted : 2026-04-21 10:15:30 UTC
  Removed  : Still connected
  Duration : 70 sec
  Status   : ACTIVE

Total sessions: 2
```

---

## License

GPL — see `usb_led_monitor.c` module header.

---
