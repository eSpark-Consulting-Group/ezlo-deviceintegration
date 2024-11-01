#!/bin/bash
HAS_AP=$(lsusb | grep -e "0bda:b720" | wc -l)
if [ $HAS_AP -eq 0 ]; then
    echo "Skip turning off AP mode, has no AP chip"
    exit 1
fi
if [ $# -eq 1 ] && [ $1 == "postpone" ]; then
    sleep 600
fi
echo "[$(date)] Turning off AP mode"
echo "[$(date)] stop hostapd"
killall hostapd
echo "[$(date)] ifconfig wlan1 down"
ifconfig wlan1 down&
echo "[$(date)] Python HTTP server stop"
killall python
