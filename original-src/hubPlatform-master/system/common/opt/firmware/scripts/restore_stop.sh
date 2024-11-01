#!/bin/bash
echo Stopping daemons before restore
/opt/firmware/bin/stop.sh except_ha_uid
