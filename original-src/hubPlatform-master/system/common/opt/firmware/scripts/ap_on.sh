#!/bin/bash
# looking for ID 0bda:b720 Realtek Semiconductor Corp. - 8723bu chip

firmware_dir=/opt/ezlo

HAS_AP=$(lsusb | grep -e "0bda:b720" | wc -l)
if [ $HAS_AP -eq 0 ]; then
    echo "Skip turning on AP mode, has no AP chip"
    exit 1
fi
RUNNING=$(ps -A | grep hostapd | wc -l)
if [ $RUNNING -eq 0 ]; then
    echo "[$(date)] Turning on AP mode"
    echo "[$(date)] ifconfig wlan1 up"
    ifconfig wlan1 up
    echo "[$(date)] hostapd start"
    hostapd -dd /etc/hostapd/hostapd.conf >> /var/log/hostapd.log 2>&1 &
    killall python
    echo "[$(date)] Python HTTP server start"
    cd $firmware_dir/scripts/
    $firmware_dir/scripts/http_cgi_server.py >> /var/log/http_cgi_server.log 2>&1 &
else
    echo "[$(date)] AP mode is already rinning. Skipping..."
fi
