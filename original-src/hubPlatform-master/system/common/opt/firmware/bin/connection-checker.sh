#!/bin/sh /etc/rc.common

interval_hours=12
hosts="mios.com google.com 1.1.1.1 8.8.8.8"
ping_interval_minutes=60

tag=firmware.connection-checker

log() {
  logger -s -t $tag "Connection checker: $1"
}

isConnected(){
    for host in $hosts;
    do
        if [ -n "$1" ]; then
          log "$1"
          if ping -I "$1" -q -w 1 -c 1 "$host" > /dev/null; then
              log "Ping Ok: $host"
              return 1
          fi
        else
          if ping -q -w 1 -c 1 "$host" > /dev/null; then
              log "Ping Ok: $host"
              return 1
          fi
        fi
    done

    log "Ping Not Ok: $host"
    return 0
}

ping_interval=$(($ping_interval_minutes * 60))
count=0  # Initialize count to 0

while :
do
    isConnected
    if [ $? -eq 1 ];  # Check for success
    then
      count=0  # Reset count on success
    else
      count=$(($count + 1))
    fi

    if [ $count -gt $(($interval_hours * (60 / $ping_interval_minutes))) ];  # Adjust the condition to check intervals
    then
      log "Hub has no connection for $interval_hours hours. Reboot from $( readlink -f "$0" ) is called"
      reboot
    fi
    log "Sleep for $ping_interval seconds"
    sleep $ping_interval
done
