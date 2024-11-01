#!/bin/bash

firmware_dir=/opt/firmware
log_dir=/var/log

set -x
if [ -f $firmware_dir/fw/firmware.dec ]; then
if [ ! "$(tar tf $firmware_dir/fw/firmware.dec)" ]; then
        echo "Firmware Archive is corrupted"
    rm -rf $firmware_dir/fw/*; sync
        exit 0;
    fi
    mkdir -p $log_dir
cd $firmware_dir/fw && tar -xf firmware.dec && $firmware_dir/fw/bin/update.sh > $log_dir/update.log 2>&1 && sync
$firmware_dir/bin/restartd.sh
rm -rf $firmware_dir/fw/*; sync
fi