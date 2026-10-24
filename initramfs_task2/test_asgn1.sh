#!/bin/sh

echo "========================================"
echo "   ASGN1 IOCTL DRIVER TEST"
echo "========================================"

echo
echo "[1] Load driver"
insmod /asgn1_driver.ko
if [ $? -ne 0 ]; then
    echo "ERROR: insmod failed"
    exit 1
fi

echo
echo "[2] Create device node"
rm -f /dev/asgn1
mknod /dev/asgn1 c 241 0
if [ $? -ne 0 ]; then
    echo "ERROR: mknod failed"
    rmmod asgn1_driver
    exit 1
fi

echo
echo "[3] Run ioctl tests"
echo "----------------------------------------"
/test_asgn1_ioctl
TEST_RESULT=$?

echo
echo "----------------------------------------"

echo
echo "[4] Check driver log"
dmesg | tail -30

echo
echo "[5] Unload driver"
rmmod asgn1_driver
if [ $? -ne 0 ]; then
    echo "ERROR: rmmod failed"
    exit 1
fi

echo
if [ $TEST_RESULT -eq 0 ]; then
    echo "========================================"
    echo "   TEST COMPLETE: PASS"
    echo "========================================"
else
    echo "========================================"
    echo "   TEST COMPLETE: FAIL"
    echo "========================================"
    exit 1
fi
