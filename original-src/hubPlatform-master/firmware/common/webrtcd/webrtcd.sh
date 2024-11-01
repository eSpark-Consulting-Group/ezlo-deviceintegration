#!/bin/sh
script_dir="$( dirname "$( readlink -f "$0" )" )"

# Wrapper to display process name starting with ha-webrtcd* so firmware can kill the process
ln -sf $script_dir/files/lib/ld-linux-armhf.so $script_dir/files/lib/ha-webrtcd_ld

exec $script_dir/files/lib/ha-webrtcd_ld --library-path $script_dir/files/lib/ $script_dir/files/bin/ha-webrtcd

