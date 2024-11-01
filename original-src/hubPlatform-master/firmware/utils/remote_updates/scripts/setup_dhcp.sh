#!/bin/sh

echo "Setting up DHCP"

killall dnsmasq
sleep 1
ifconfig eth0 down
sleep 3
ifconfig eth0 192.168.123.1 netmask 255.255.255.0 up

sleep 5
dnsmasq \
  --port=0 \
  --interface=eth0 \
  --dhcp-option=3,192.168.123.1 \
  --dhcp-option=6,8.8.8.8 \
  --dhcp-range=192.168.123.2,192.168.123.2 \
  --leasefile-ro \
  --dhcp-authoritative

