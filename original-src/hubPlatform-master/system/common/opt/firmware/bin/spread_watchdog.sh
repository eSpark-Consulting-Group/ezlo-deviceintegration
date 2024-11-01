#!/bin/sh

SPREAD_PORT=4803
CHECK_INTERVAL=60

sleep_for () {
  for f in $(seq $1); do
    sleep 1
  done
}

is_spread_running () {
  pgrep -f 'spread -c' > /dev/null 2>&1
  return $?
}

is_spread_port_opened () {
  lsof -b -i:$SPREAD_PORT -sTCP:LISTEN > /dev/null 2>&1
  return $?
}

check_spread_started () {
    local timeout=60
    for f in $(seq $timeout); do
        if is_spread_running ; then
            return 0
        fi
        sleep 1
    done
    return 1
}

check_port_opened () {
    local timeout=60
    for f in $(seq $timeout); do
        if is_spread_port_opened ; then
            return 0
        fi
        sleep 1
    done
    return 1
}

while true
do
    if ! check_spread_started ; then
       echo "$(date +"%Y-%M-%d %T %z") Spread service was not started! Rebooting ..."
       sleep 2
       reboot -f
    fi

    if ! check_port_opened ; then
       echo "$(date +"%Y-%M-%d %T %z") Spread port was not opened! Rebooting ..."
       sleep 2
       reboot -f
    fi

    sleep_for $CHECK_INTERVAL
done

