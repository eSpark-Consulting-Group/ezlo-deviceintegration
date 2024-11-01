#!/bin/bash

echo 0 > /sys/class/gpio/gpio12_ph2/value
/bin/sleep 1
echo 1 > /sys/class/gpio/gpio12_ph2/value
/bin/sleep 2
killall ha-ledd
/bin/sleep 2
echo 1 > /sys/class/leds/ready_yellow_ph10/brightness
/sbin/ifconfig wlan1 down
/sbin/dhclient wlan0 &


