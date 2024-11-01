#!/bin/sh

# Init binaries
UP_CMD="updatecmd"
MIG_TOOL="migration_tool"
LOG_CMD="logcmd"

LOG_DIR="/var/log/firmware"
FIRMWARE_DIR="/opt/firmware"

check_factorytest() {
  if [ -e /home/.factorytest_required ];
  then
    echo ".factorytest_required flag detected - firmware daemons will not start!" > /dev/kmsg
    exit 0
  fi
}

locate_bin() {
  bin_file=$1
  bin_location=$(cd $FIRMWARE_DIR && find . -type f -name "$bin_file" -print0 | cut -d / -f2-)
  [ -z "$bin_location" ] && exit

  case "$bin_file" in
    "logcmd")
      LOG_CMD="$bin_location";;
    "updatecmd")
      UP_CMD="$bin_location";;
    "migration_tool")
      MIG_TOOL="$bin_location";;
  esac
}

start_service() {
  echo "Starting service: $1"
  $1 start >/dev/null 2>&1
}

base_services="system_monitor spread mosquitto"
# If system in real hub let procd control services
if command -v procd >/dev/null 2>&1; then
  check_factorytest

  test -x /usr/sbin/led_cmd && /usr/sbin/led_cmd --take-control

  # Run initialize binaries
  locate_bin $UP_CMD
  $FIRMWARE_DIR/$UP_CMD

  locate_bin $MIG_TOOL
  $FIRMWARE_DIR/$MIG_TOOL >> "$LOG_DIR/migration.log" 2>&1

  locate_bin $LOG_CMD
  ( $FIRMWARE_DIR/$LOG_CMD --update-settings 1> /dev/null )&
  sync

  for f in ${base_services}; do
    start_service /etc/init.d/$f
  done

  # Start core services
  core_services=$(ls -d /etc/init.d/ha-* | grep -E 'ha-[0-9]{2}')
  for f in ${core_services}; do
    start_service $f
  done

  # Start noncore services
  noncore=$(ls -d /etc/init.d/ha-* | grep -vE 'ha-[0-9]{2}')
  for f in ${noncore}; do
    start_service $f
  done

  if ! ps | grep -q "[s]pread_watchdog.sh"; then
    echo  "Starting spread_watchdog.sh"
    ("$FIRMWARE_DIR"/bin/spread_watchdog.sh >> "$LOG_DIR/spread_watchdog.log" 2>&1 )&
  fi

  /etc/init.d/avahi-daemon restart
  sync
  exit 0
fi

# This section will work on vhub
firmware_dir="$( dirname "$( dirname "$( readlink -f "$0" )" )" )"
hadir=$firmware_dir/bin
data_dir=$firmware_dir/data
scripts_dir=$firmware_dir/scripts
addons_dir=$hadir/addons
services_dir=$firmware_dir/services
log_dir="$( dirname "$( dirname "$firmware_dir" )" )/var/log/firmware"

while [ $# -gt 0 ]
do
    case "$1" in
        -l|--logdir) log_dir="$2";  shift 2 ;;
        -d|--daemon) daemon="$2";   shift 2 ;;
        *)
            echo "$1 is not an option"
            exit 1
            ;;
    esac
done

init () {
  mkdir -p "$log_dir"
  mkdir -p "$scripts_dir"
  mkdir -p "$addons_dir"

  if [ -L "$data_dir" ]
  then
    mkdir -p /home/data
  else
    # for local use
    mkdir -p "$data_dir"
  fi

  if [ ! -d "$data_dir/custom_plugins" ]
  then
    mkdir -p "$data_dir/custom_plugins"
  fi

  check_factorytest

  echo "Check if there are pending updates."
  "$hadir"/updatecmd

  # Migrate data if needed
  "$hadir"/migration_tool >> "$log_dir/migration.log" 2>&1

  # Flush file caches to filesystem after update and migration
  sync

  # Schedule logrotate
  ( "$hadir"/logcmd --update-settings 1> /dev/null )&


  cd "$hadir/addons" || return
  for file in $(find . -maxdepth 1 -type f ! -name "*.*" -name "ha-*" ! -name "ha-*cmd"); do
    if [ ! -f "$hadir/${file}.sh" ];
    then
      ln -sf "$hadir/hz-daemon.sh" "$hadir/${file}.sh";
    fi
  done

  cd "$hadir" || return
  for file in $(find . -maxdepth 1 -type f ! -name "*.*" -name "ha-*" ! -name "ha-*cmd"); do
    if [ ! -f "$hadir/${file}.sh" ];
    then
      ln -sf "$hadir/hz-daemon.sh" "$hadir/${file}.sh";
    fi
  done

  for service in $(find "$services_dir" -mindepth 2 -maxdepth 2 -type f ! -name "*.*" -name "ha-*" ! -name "ha-*cmd"); do
    file="$(basename "$service")"
    if [ ! -f "$hadir/${file}.sh" ];
    then
      ln -sf "$hadir/hz-daemon.sh" "$hadir/${file}.sh";
    fi
  done
}

#start daemon script
start_ds () {
  pid="$(pgrep -f "$1")"
  printname="$(basename "$1")"
  if [ -n "$pid" ]
  then
    echo "Skipping $printname: it's already running with pid $pid"
    return;
  fi
  echo  "Starting $printname"
  ($1 -l "$log_dir" > /dev/null 2>&1 )&
}

#start daemon
start_addons () {
  for i in "$addons_dir"/ha-*
  do
    script="$hadir/$(basename "$i").sh"
    start_ds "$script"
  done
}

#start services
start_services () {
  for service in $(find "$services_dir" -mindepth 2 -maxdepth 2 -type f ! -name "*.*" -name "ha-*" ! -name "ha-*cmd"); do
    script="$hadir/$(basename "$service").sh"
    start_ds "$script"
  done
}

#start core daemon
start_core_daemons () {
  # NOTE: keep in reverse order with stop.sh / kill_core_daemons
  start_ds "$hadir/ha-backupd.sh"
  start_ds "$hadir/ha-cored.sh"
  start_ds "$hadir/ha-hardware_manager.sh"
  start_ds "$hadir/ha-networkd.sh"
  start_ds "$hadir/ha-uid.sh"
  start_ds "$hadir/ha-update_manager.sh"
  start_ds "$hadir/ha-cloud_devices.sh"
  start_ds "$hadir/ha-scenes_manager.sh"
  start_ds "$hadir/ha-extensions_manager.sh"
  start_ds "$hadir/ha-external_controllers.sh"
}

#start specific addon/service if not already started
start_specific_component () {
  if { [ -f "$addons_dir/$1" ] || [ -f "$hadir/$1" ] ; } && [ -f "$hadir/$1.sh" ];
  then
    start_ds "$hadir/$1.sh"
  fi
}

#start migration tool first was done here

if [ -z "$daemon" ]
then
  test -x /usr/sbin/led_cmd && /usr/sbin/led_cmd --take-control
  init
#start all
#check if spread is running and start if it is not
  pid_spread="$(pgrep spread)"
  if [ -n "$pid_spread" ]
  then
    echo "Skipping spread.sh: it's already running with pid $pid_spread"
  else
    echo  "Starting spread.sh"
    ("$hadir"/spread.sh -l "$log_dir" >> "$log_dir/spread.log" 2>&1 )&
    echo  "Starting spread_watchdog.sh"
    ("$hadir"/spread_watchdog.sh >> "$log_dir/spread_watchdog.log" 2>&1 )&
  fi

#check if mqtt broker is running and start if it is not
  pid_mosquitto="$(pgrep -x mosquitto.sh)"
  if [ -n "$pid_mosquitto" ]
  then
    echo "Skipping mosquitto.sh: it's already running with pid $pid_mosquitto"
  else
    echo  "Starting mosquitto.sh"
    ("$hadir"/mosquitto.sh -l "$log_dir" >> "$log_dir/mosquitto.log" 2>&1 )&
  fi
  sleep 1
#start all daemons in predefined sequence
  start_core_daemons
  sleep 1
  start_addons
  sleep 1
  start_services
else
  start_specific_component "$daemon"
fi

# Flush file caches to filesystem after services started
sync

exit 0
