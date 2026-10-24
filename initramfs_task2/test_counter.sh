#!/bin/sh

FAIL=0

echo "========================================"
echo "      COUNTER DRIVER TEST"
echo "========================================"

echo
echo "[1] Prepare /dev"
if mountpoint -q /dev; then
    echo "PASS: /dev is already mounted"
else
    mount -t devtmpfs devtmpfs /dev 2>/dev/null
    if mountpoint -q /dev; then
        echo "PASS: devtmpfs mounted"
    else
        echo "FAIL: cannot mount devtmpfs"
        FAIL=1
    fi
fi

echo
echo "[2] Load driver"
if insmod /counter_driver.ko; then
    echo "PASS: driver loaded"
else
    echo "FAIL: driver load failed"
    FAIL=1
fi

echo
echo "[3] Check device and proc entry"

if [ -c /dev/counter ]; then
    echo "PASS: /dev/counter is a character device"
else
    echo "FAIL: /dev/counter is not a character device"
    FAIL=1
fi

if [ -e /proc/counter_info ]; then
    echo "PASS: /proc/counter_info exists"
else
    echo "FAIL: /proc/counter_info missing"
    FAIL=1
fi

echo
echo "[4] Initial read: expected 0"
VALUE=$(cat /dev/counter)
echo "READ: $VALUE"

if [ "$VALUE" = "0" ]; then
    echo "PASS: initial counter = 0"
else
    echo "FAIL: initial counter != 0"
    FAIL=1
fi

echo
echo "[5] Second read: expected 1"
VALUE=$(cat /dev/counter)
echo "READ: $VALUE"

if [ "$VALUE" = "1" ]; then
    echo "PASS: second counter = 1"
else
    echo "FAIL: second counter != 1"
    FAIL=1
fi

echo
echo "[6] Same FD / *ppos test"

exec 3</dev/counter

FIRST_VALUE=$(cat <&3)
FIRST_STATUS=$?

SECOND_VALUE=$(cat <&3)
SECOND_STATUS=$?

exec 3<&-

echo "FIRST_READ:  $FIRST_VALUE"
echo "SECOND_READ: $SECOND_VALUE"
echo "FIRST_STATUS:  $FIRST_STATUS"
echo "SECOND_STATUS: $SECOND_STATUS"

if [ "$FIRST_STATUS" -eq 0 ] && [ "$SECOND_STATUS" -eq 0 ] && \
   [ -n "$FIRST_VALUE" ] && [ -z "$SECOND_VALUE" ]; then
    echo "PASS: *ppos prevents repeated read"
else
    echo "FAIL: *ppos test failed"
    FAIL=1
fi

echo
echo "[7] Write 100, then read: expected 100"

if echo 100 > /dev/counter; then
    echo "PASS: write 100 succeeded"
else
    echo "FAIL: write 100 failed"
    FAIL=1
fi

VALUE=$(cat /dev/counter)
echo "READ: $VALUE"

if [ "$VALUE" = "100" ]; then
    echo "PASS: counter = 100"
else
    echo "FAIL: counter != 100"
    FAIL=1
fi

echo
echo "[8] Negative value: expected failure"

if echo -5 > /dev/counter 2>/dev/null; then
    echo "FAIL: negative value was accepted"
    FAIL=1
else
    echo "PASS: negative value rejected"
fi

echo
echo "[9] Non-integer value: expected failure"

if echo abc > /dev/counter 2>/dev/null; then
    echo "FAIL: non-integer value was accepted"
    FAIL=1
else
    echo "PASS: non-integer value rejected"
fi

echo
echo "[10] Check /proc/counter_info"
cat /proc/counter_info

echo
echo "[11] Unload driver"

if rmmod counter_driver; then
    echo "PASS: driver unloaded"
else
    echo "FAIL: driver unload failed"
    FAIL=1
fi

echo
echo "========================================"

if [ "$FAIL" -eq 0 ]; then
    echo "   TEST COMPLETE: PASS"
else
    echo "   TEST COMPLETE: FAIL"
fi

echo "========================================"

exit "$FAIL"
