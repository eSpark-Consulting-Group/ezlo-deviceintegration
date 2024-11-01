#!/bin/bash

firmware_dir=/opt/firmware

BAT_FLAG=$(/bin/cat $firmware_dir/etc/batteryFlag)
echo ${BAT_FLAG} >> /root/log.tmp
if [ $1 -eq 1 ] && [ $BAT_FLAG -eq 0 ]; then
    echo 1 > $firmware_dir/etc/batteryFlag
    $firmware_dir/scripts/reloadWifi.sh
    $firmware_dir/bin/ha-infocmd batteryInfo {"isOnBattery":true}
elif [ $BAT_FLAG -eq 1 ]; then
    echo 0 > $firmware_dir/etc/batteryFlag
    $firmware_dir/bin/ha-infocmd batteryInfo {"isOnBattery":false}
fi