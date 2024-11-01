#!/bin/bash

# TODO: let plugin_package_checker to perform scripts compilation to move this logic to c++...

package_file=$1
results_file=$2

printResults() {
    level=$1
    message=$2

    echo "{ \"error\": { \"level\": \"${level}\", \"msg\": \"${message}\" } }" > $results_file
} 

./plugin_package_checker $package_file $results_file
if [ $? != 0 ]; then
    echo "Failure during plugin package check"
    exit 1
fi

tmp_dir=$(mktemp -d --tmpdir=.)
if [ $? != 0 ]; then
    echo "Failed to create a temporary directory"
    exit 1
fi

tar -C $tmp_dir -xzf $1
if [ $? != 0 ]; then
    msg="Failed to untar a package"

    echo $msg
    printResults "app" $msg
    rm -rf $tmp_dir

    exit 1
fi

HubPluginsUtils/opkg/compile_lua_scripts.py directories $tmp_dir > $tmp_dir/results
if [ $? != 0 ]; then
    echo "Failed to compile plugin scripts"
    printResults "invalid_scripts" "$(cat $tmp_dir/results)"
    rm -rf $tmp_dir

    exit 1
fi

rm -rf $tmp_dir
exit 0
