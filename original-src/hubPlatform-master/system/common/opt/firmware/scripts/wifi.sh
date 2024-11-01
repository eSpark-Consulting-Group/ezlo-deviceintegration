#!/bin/sh
modprobe gpio-sunxi
# wifi
echo 12 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio12_ph2/direction 
echo 1 > /sys/class/gpio/gpio12_ph2/value  
/sbin/depmod
#modprobe 8188eu
#sleep 15
#ifconfig wlan0 up
#sleep 10
#wpa_supplicant -D wext -i wlan0 -c/etc/wpa_supplicant.conf -B -f/var/log/wifi.log
#/opt/firmware/bin/ha-ledcmd -s "{\"indicator\":\"wifi\",\"setState\":1}"
