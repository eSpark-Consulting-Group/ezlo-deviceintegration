#!/bin/sh

wait_service_stop() {
  while $1 status | grep -q "running"; do
    echo "Waiting to stop service: $1"
    sleep 1
  done
}

stop_service() {
  echo "Stopping service: $1"
  $1 stop >/dev/null 2>&1
  wait_service_stop $1
}

backup_flag=0

while [ $# -gt 0 ]
do
    case "$1" in
        -b|--backup) backup_flag=1;  shift 1 ;;
        # TODO: Clean this parameter. Procd(service) can kill specific daemon.
        -d|--daemon) daemon="$2";   shift 2 ;;
        *)
            echo "$1 is not an option"
            exit 1
            ;;
    esac
done

# Stop base services in reverse order
base_services="mosquitto spread system_monitor"
# If system in real hub let procd control services
if command -v procd >/dev/null 2>&1; then

  spread_pid=$(ps | grep "[s]pread_watchdog.sh" | awk '{print $1}')
  [ -n "$spread_pid" ] && kill -9 "$spread_pid"

  # Stop noncore
  noncore=$(ls -d /etc/init.d/ha-* | grep -vE 'ha-[0-9]{2}')
  for f in ${noncore}; do
    stop_service $f
  done

  core_services_reverse=$(ls -d /etc/init.d/ha-* | grep -E 'ha-[0-9]{2}' | sort -r)
  for f in ${core_services_reverse}; do
    if [ $backup_flag = 1 ]; then
      if echo "$f" | grep -qwE ".*(ha-01_backupd).*"; then
        echo "Skipping $f - cannot be killed for backup"
        continue
      fi
    fi
    stop_service $f
  done

  for f in ${base_services}; do
    if [ $backup_flag = 1 ]; then
      if echo "$f" | grep -qwE ".*(spread).*"; then
        echo "Skipping $f - cannot be killed for backup"
        continue
      fi
    fi
    stop_service /etc/init.d/$f
  done

  pidof updatecmd || (test -x /usr/sbin/led_cmd && /usr/sbin/led_cmd --release-control)

  exit 0
fi

firmware_dir="$( dirname "$( dirname "$( readlink -f "$0" )" )" )"
hadir=$firmware_dir/bin
addons_dir=$hadir/addons
services_dir=$firmware_dir/services

centisleep () {
  if command -v usleep >/dev/null 2>&1; then
    usleep 10000
  else
    sleep 0.01
  fi
}

kill_sh () {
  kill_ds "/bin/sh $1" -9 "$1"
}

#kill daemon or script
kill_ds () {
  if [ $backup_flag = 1 ]
  then
    if echo "$1" | grep -qwE ".*(ha-backupd|spread).*"
    then
        echo "Skipping $1 - cannot be killed for backup"
        return 0
    fi
  fi
  PID=$(pgrep -f "$1")
  printname="$(basename "${3:-$1}")"
  signal=${2:--15}
  if [ -z "$PID" ]
  then
      echo "Skipping $printname"
  else
    for each_pid in $PID; do
      echo "Killing $each_pid $printname"
      kill "$signal" "$each_pid" > /dev/null 2>&1
      centisleep
      counter=0
      while kill -0 "$each_pid" 2> /dev/null;
      do
        counter=$((counter+1))
        if [ $counter -gt 7 ]
        then
          echo "[$counter] Trying force kill"
          kill -9 "$each_pid"
          centisleep
        else
          echo "[$counter] Waiting"
          sleep 1
        fi
      done
    done
  fi
}

kill_specific_addon () {
  if [ -f "$addons_dir/$1" ];
  then
    kill_sh "$hadir/$1.sh"
    kill_ds "$addons_dir/$1"
  fi
}

#shut down addons ( formly )
kill_addons () {
  for i in "$addons_dir"/ha-*
  do
    kill_sh "$hadir/$(basename "$i").sh"
    kill_ds "$i"
  done
}

#shut down services
kill_services () {
  for service in $(find "$services_dir" -mindepth 2 -maxdepth 2 -type f ! -name "*.*" -name "ha-*" ! -name "ha-*cmd"); do
    ha_daemon="$(basename "$service")"
    kill_sh "$hadir/$ha_daemon.sh"
    kill_ds "$ha_daemon"
  done
}

#shut down core daemons
kill_core_daemons () {
  # NOTE: keep in reverse order with start.sh / start_core_daemons

  kill_sh "$hadir/ha-external_controllers.sh"
  kill_ds "$hadir/ha-external_controllers"

  kill_sh "$hadir/ha-extensions_manager.sh"
  kill_ds "$hadir/ha-extensions_manager"

  kill_sh "$hadir/ha-scenes_manager.sh"
  kill_ds "$hadir/ha-scenes_manager"

  kill_sh "$hadir/ha-cloud_devices.sh"
  kill_ds "$hadir/ha-cloud_devices"

  kill_sh "$hadir/ha-update_manager.sh"
  kill_ds "$hadir/ha-update_manager"

  kill_sh "$hadir/ha-uid.sh"
  kill_ds "$hadir/ha-uid"

  kill_sh "$hadir/ha-networkd.sh"
  kill_ds "$hadir/ha-networkd"

  kill_sh "$hadir/ha-hardware_manager.sh"
  kill_ds "$hadir/ha-hardware_manager"

  kill_sh "$hadir/ha-cored.sh"
  kill_ds "$hadir/ha-cored"

  kill_sh "$hadir/ha-backupd.sh"
  kill_ds "$hadir/ha-backupd"
}

# if there are script parameters - so killing specific addon
if [ -z "$daemon" ]
then
  kill_services
  kill_addons
  kill_core_daemons

  kill_sh "$hadir/mosquitto.sh"
  kill_ds "mosquitto"

  kill_sh "$hadir/spread_watchdog.sh"
  kill_sh "$hadir/spread.sh"
  kill_ds "spread"

  pidof updatecmd || (test -x /usr/sbin/led_cmd && /usr/sbin/led_cmd --release-control)
else
  kill_specific_addon "$daemon"
fi

exit 0
