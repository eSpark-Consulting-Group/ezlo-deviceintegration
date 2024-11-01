# ash script to calculate memory usage of processes, uptime of processes and cgroup webrtc state.
# Version: 0.2
# Based on start.sh script.

# set -x

which_shell() {
    if [ "$SHELL" = "/bin/ash" ]; then
        echo "ash"
    elif [ "$SHELL" = "/bin/bash" ]; then
        echo "bash"
    else
        echo "sh"
    fi
}

which_hub() {
    local hadir="$1"
    echo "Hub type: $($hadir/at88util --type)"
    echo "hub serial: $($hadir/at88util --serial)"
    echo "Firmware version: $($hadir/at88util --version)"
}

show_process_stat() {
    local pid="$1"
    local stat_file="/proc/$pid/stat"
    echo "show_process_stat $pid"

    if [ -e "$stat_file" ]; then
        local stat_data=$(<"$stat_file")
        echo "Process $pid statistics:"
        echo "$stat_data"
    else
        echo "Process with PID $pid not found or no permission to access /proc/$pid/stat"
    fi
}

get_process_uptime() {
    local pid="$1"
    local boot_time_seconds=$(awk '/btime/ {print $2}' /proc/stat)
    local process_start_time_ticks=$(awk '{print $22}' /proc/$pid/stat)
    # local clock_ticks_per_second=$(($(getconf CLK_TCK 2>/dev/null) || echo 100))
    local clock_ticks_per_second=100 # hardcoded in ash

    local process_start_time_seconds=$((process_start_time_ticks / clock_ticks_per_second))
    local current_time_seconds=$(date +%s)

    local uptime_seconds=$((current_time_seconds - boot_time_seconds - process_start_time_seconds))
    echo "Process $pid uptime: $uptime_seconds seconds"
}

service_status_command() {
    local service_name="$1"
    local service_pids=$(pgrep "$service_name")
    if [ -z "$service_pids" ]; then
        echo "Service $service_name is not running"
    else
        echo "$service_pids" | while read -r service_pid; do
            local status_file="/proc/$service_pid/status"
            if [ -e "$status_file" ]; then
                local output=$(cat "$status_file" | grep -E 'Name|VmRSS|RssAnon|State|Pid|Threads')
                echo "================================"
                echo "Status file: $status_file"
                echo "Service status: $service_name (PID: $service_pid)"
                echo "Status: $output"
                get_process_uptime "$service_pid"
            else
                echo "Status file for PID $service_pid not found"
            fi
        done
    fi
}

# Function to check cgroup status
cgroup_status() {
    echo "================================"
    # Check if the necessary groups are installed
    if ! command -v cgget &>/dev/null; then
        echo "cgroup tools are not installed"
        exit 1
    fi

    local cgroup_name="$1"
    local cgroup_webrtc_status="cgget -g memory:$cgroup_name | grep -E 'memory\.usage_in_bytes|memory\.limit_in_bytes|oom_kill'"
    local output=$(eval "$cgroup_webrtc_status")
    echo "================================"
    echo "CGroup status: $cgroup_name"
    echo "Status: $output"
}

# === Main part ===
# Get status of core services, addons, services, cgroups

firmware_dir="$( dirname "$( dirname "$( readlink -f "$0" )" )" )"
hadir=$firmware_dir/bin
data_dir=$firmware_dir/data
scripts_dir=$firmware_dir/scripts
addons_dir=$hadir/addons
services_dir=$firmware_dir/services

echo "$(date +'%F %T%z') Script is running under the shell: $(which_shell), hub type: $(which_hub "$hadir")"
echo "$(date +'%F %T%z') Hub info: $(which_hub "$hadir")"

cd "$hadir" || return
for file in $(find . -maxdepth 1 -type f ! -name "*.*" -name "ha-*" ! -name "ha-*cmd"); do
    basename="$(basename $file)"
    if [ -f "$hadir/${basename}" ]; then
        echo "call service_status_command $basename"
        service_status_command "$basename"
    fi
done

cd "$addons_dir" || return
for file in $(find . -maxdepth 1 -type f ! -name "*.*" -name "ha-*" -o -name "mediamtx" ! -name "ha-*cmd" ); do
    basename="$(basename $file)"
    if [ -f "$addons_dir/${basename}" ]; then
        echo "call service_status_command $basename"
        service_status_command "$basename"
    fi
done

if [ -d "$services_dir" ]; then
    cd "$services_dir" || return
    for service in $(find "$services_dir" -mindepth 2 -maxdepth 4 -type f ! -name "*.*" -name "ha-*" ! -name "ha-*cmd" ! -name "ha-webrtcd_ld"); do
        basename="$(basename "$service")"
        service_status_command "$basename"
    done
fi

cd "$hadir" || return
for file in $(find . -maxdepth 1 -type f -name "*cmd"); do
    basename="$(basename $file)"
    if [ -f "$hadir/${basename}" ]; then
        echo "call service_status_command $basename"
        service_status_command "$basename"
    fi
done

echo "Base services: spread mosquitto system_monitor"

# Loop through each utility and check its status
base_services='spread mosquitto system_monitor'
for base_service in $base_services; do
    service_status_command "$base_service"
done

# List of utilities to check
utilities="ip netifd pppd wpa_supplicant odhcpcd udhcpc dnsmasq ifconfig ntpd ssh curl wget opkg"

echo "Network stack utilities: $utilities"

# Loop through each utility and check its status
for utility in $utilities; do
    service_status_command "$utility"
done

# Check cgroup status
cgroup_status "webrtc"
cgroup_status "hub-log-sender"

echo "$(date +'%F %T%z') Firmware status collection finished"
echo "$(date +'%F %T%z') ================================================================="
