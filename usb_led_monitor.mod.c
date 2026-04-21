#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xf46d5bf3, "mutex_lock" },
	{ 0x37602407, "usb_get_dev" },
	{ 0xf46d5bf3, "mutex_unlock" },
	{ 0x6c00f410, "usb_control_msg" },
	{ 0xa31fd687, "usb_put_dev" },
	{ 0xcb8b6ec6, "kfree" },
	{ 0xbd03ed67, "__ref_stack_chk_guard" },
	{ 0x680628e7, "ktime_get_real_ts64" },
	{ 0xe1e1f979, "_raw_spin_lock_irqsave" },
	{ 0x81a1a811, "_raw_spin_unlock_irqrestore" },
	{ 0x90a48d82, "__ubsan_handle_out_of_bounds" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0xe931a49e, "single_open" },
	{ 0xbd03ed67, "random_kmalloc_seed" },
	{ 0xfaabfe5e, "kmalloc_caches" },
	{ 0xc064623f, "__kmalloc_cache_noprof" },
	{ 0x49733ad6, "queue_work_on" },
	{ 0x2352b148, "timer_delete_sync" },
	{ 0xbeb1d261, "__flush_workqueue" },
	{ 0x47886e07, "usb_unregister_notify" },
	{ 0xc0f19660, "remove_proc_entry" },
	{ 0x40a621c5, "snprintf" },
	{ 0x945fda2d, "usb_string" },
	{ 0x9479a1e8, "strnlen" },
	{ 0xd70733be, "sized_strscpy" },
	{ 0xe54e0a6b, "__fortify_panic" },
	{ 0x2ecc6b55, "rtc_time64_to_tm" },
	{ 0xb61837ba, "seq_printf" },
	{ 0xaa9a3b35, "seq_read" },
	{ 0x253f0c1d, "seq_lseek" },
	{ 0x34d5450c, "single_release" },
	{ 0xd272d446, "__fentry__" },
	{ 0xe8213e80, "_printk" },
	{ 0xdf4bee3d, "alloc_workqueue_noprof" },
	{ 0x80222ceb, "proc_create" },
	{ 0x1404c363, "usb_for_each_dev" },
	{ 0x47886e07, "usb_register_notify" },
	{ 0x02f9bbf0, "timer_init_key" },
	{ 0x058c185a, "jiffies" },
	{ 0x32feeafc, "mod_timer" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0xbeb1d261, "destroy_workqueue" },
	{ 0xbebe66ff, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0xf46d5bf3,
	0x37602407,
	0xf46d5bf3,
	0x6c00f410,
	0xa31fd687,
	0xcb8b6ec6,
	0xbd03ed67,
	0x680628e7,
	0xe1e1f979,
	0x81a1a811,
	0x90a48d82,
	0xd272d446,
	0xe931a49e,
	0xbd03ed67,
	0xfaabfe5e,
	0xc064623f,
	0x49733ad6,
	0x2352b148,
	0xbeb1d261,
	0x47886e07,
	0xc0f19660,
	0x40a621c5,
	0x945fda2d,
	0x9479a1e8,
	0xd70733be,
	0xe54e0a6b,
	0x2ecc6b55,
	0xb61837ba,
	0xaa9a3b35,
	0x253f0c1d,
	0x34d5450c,
	0xd272d446,
	0xe8213e80,
	0xdf4bee3d,
	0x80222ceb,
	0x1404c363,
	0x47886e07,
	0x02f9bbf0,
	0x058c185a,
	0x32feeafc,
	0xd272d446,
	0xbeb1d261,
	0xbebe66ff,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"mutex_lock\0"
	"usb_get_dev\0"
	"mutex_unlock\0"
	"usb_control_msg\0"
	"usb_put_dev\0"
	"kfree\0"
	"__ref_stack_chk_guard\0"
	"ktime_get_real_ts64\0"
	"_raw_spin_lock_irqsave\0"
	"_raw_spin_unlock_irqrestore\0"
	"__ubsan_handle_out_of_bounds\0"
	"__stack_chk_fail\0"
	"single_open\0"
	"random_kmalloc_seed\0"
	"kmalloc_caches\0"
	"__kmalloc_cache_noprof\0"
	"queue_work_on\0"
	"timer_delete_sync\0"
	"__flush_workqueue\0"
	"usb_unregister_notify\0"
	"remove_proc_entry\0"
	"snprintf\0"
	"usb_string\0"
	"strnlen\0"
	"sized_strscpy\0"
	"__fortify_panic\0"
	"rtc_time64_to_tm\0"
	"seq_printf\0"
	"seq_read\0"
	"seq_lseek\0"
	"single_release\0"
	"__fentry__\0"
	"_printk\0"
	"alloc_workqueue_noprof\0"
	"proc_create\0"
	"usb_for_each_dev\0"
	"usb_register_notify\0"
	"timer_init_key\0"
	"jiffies\0"
	"mod_timer\0"
	"__x86_return_thunk\0"
	"destroy_workqueue\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "7B994C41CB4FDE039EA8751");
