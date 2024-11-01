#!/bin/bash

#copy HubPluginsUtils
#copy plugin_package_checker binary 
#copy Dockerfile
#build an image
#clean resourses

docker_image_name="plugin_package_checker"
docker_image_version="1.0.0"
build_dir="build/amd_release/plugin_package_checker_docker"
hub_plugins_utils_dir="deps/HubPlugins/utils/deps"
utility_path="build/amd_release/bin/plugin_package_checker"
dokerfiles_dir="misc/dockers/plugin_packages_checker"

mkdir -p $build_dir

cp -r $hub_plugins_utils_dir $build_dir/HubPluginsUtils
cp $utility_path $build_dir
cp -r $dokerfiles_dir/* $build_dir

docker build -t "$docker_image_name:$docker_image_version" $build_dir

rm -rf $build_dir
