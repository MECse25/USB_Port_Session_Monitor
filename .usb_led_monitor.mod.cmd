savedcmd_usb_led_monitor.mod := printf '%s\n'   usb_led_monitor.o | awk '!x[$$0]++ { print("./"$$0) }' > usb_led_monitor.mod
