#!/bin/bash
# test_usb_led.sh — Load, test, and unload the USB LED Monitor module
# Works on Esrijan DDK v2.1 (PGM LED blinks based on connected USB devices)

set -e

MODULE="usb_led_monitor"
KO="${MODULE}.ko"

echo "============================================"
echo "  USB LED Monitor — DDK v2.1 PGM LED Test"
echo "============================================"
echo ""

# ── sanity checks ────────────────────────────────────────────────
if [ ! -f "$KO" ]; then
    echo "[ERROR] $KO not found. Run 'make' first."
    exit 1
fi

if lsmod | grep -q "$MODULE"; then
    echo "[INFO] Module already loaded. Removing first..."
    sudo rmmod "$MODULE"
    sleep 1
fi

# ── detect LED path ──────────────────────────────────────────────
LED_PATH="/sys/class/leds/ddk::pgm"
if [ -d "$LED_PATH" ]; then
    echo "[OK]  Found PGM LED at $LED_PATH"
    echo "      Current brightness: $(cat $LED_PATH/brightness)"
    echo "      Current trigger   : $(cat $LED_PATH/trigger 2>/dev/null || echo 'n/a')"
else
    echo "[WARN] $LED_PATH not found."
    echo "       If your board uses a different LED name, check:"
    echo "         ls /sys/class/leds/"
    echo "       Then load with: sudo insmod $KO pgm_gpio=<GPIO_NUMBER>"
fi
echo ""

# ── load module ──────────────────────────────────────────────────
echo "[STEP 1] Loading module..."
sudo insmod "$KO"
sleep 1

echo "[CHECK] Kernel log (last 8 lines):"
dmesg | tail -8
echo ""

# ── show current status ──────────────────────────────────────────
echo "[STEP 2] Current session log (no devices plugged yet):"
cat /proc/usb_monitor
echo ""

# ── interactive test ─────────────────────────────────────────────
echo "------------------------------------------------------------"
echo "  NOW PLUG IN USB DEVICES AND WATCH THE PGM LED:"
echo "    1 device  → LED blinks ONCE  per cycle"
echo "    2 devices → LED blinks TWICE per cycle"
echo "    3 devices → LED blinks THREE times per cycle"
echo "------------------------------------------------------------"
echo ""
echo "Waiting 20 seconds for you to plug/unplug USB devices..."
echo "(Press Ctrl+C to skip the wait)"
echo ""

for i in $(seq 20 -1 1); do
    printf "\r  Time remaining: %2d sec  " "$i"
    sleep 1
done
echo ""
echo ""

# ── final log ────────────────────────────────────────────────────
echo "[STEP 3] Final session log:"
cat /proc/usb_monitor
echo ""

echo "[STEP 4] Kernel log (last 15 lines):"
dmesg | tail -15
echo ""

# ── unload ───────────────────────────────────────────────────────
echo "[STEP 5] Unloading module..."
sudo rmmod "$MODULE"
sleep 1

echo ""
echo "[DONE] Test complete. PGM LED should now be off."
echo "       Kernel log:"
dmesg | tail -3
