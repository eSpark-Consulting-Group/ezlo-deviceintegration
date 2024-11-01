#!/bin/sh

release_name="$(date +'remote_updates_%Y.%m.%d')"
working_dir="releases/"$release_name"_working_dir"

deploy_path_root="$working_dir/remote_updates"
deploy_path_bin="$deploy_path_root/bin"

echo $release_name

mkdir -p $deploy_path_root
mkdir -p $deploy_path_bin
mkdir -p $deploy_path_root/files

cp -r service_files $deploy_path_root/

cp scripts/install.sh $deploy_path_root/
cp scripts/led_cmd.sh $deploy_path_bin/
cp scripts/setup_dhcp.sh $deploy_path_bin/
cp scripts/remote_updates_start.sh $deploy_path_bin/

cp settings.json $deploy_path_bin/

service_binary_path="$deploy_path_bin/remote_updates"
env GOOS=linux GOARCH=arm go build -ldflags="-s -w" -o $service_binary_path remote_updates.go
chmod a+x $service_binary_path

tar cvzf releases/$release_name".tgz" -C $working_dir remote_updates

