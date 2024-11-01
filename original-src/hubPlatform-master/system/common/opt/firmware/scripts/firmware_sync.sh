#!/bin/sh
echo "syncing filesystem"
sync
killall -9 plymouthd
echo "shutting down HA"
/opt/firmware/bin/stop.sh
echo "killing dhclient"
killall -9 dhclient
echo "syncing fs"
sync
