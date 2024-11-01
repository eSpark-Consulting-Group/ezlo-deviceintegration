#!/bin/sh

. /lib/functions/check_possibility_to_upgrade.sh

# Script supports:
#   - uncompressed images (recovery.bin)
#   - gzip compressed images (recovery.bin.gz)
#   - bzip2 compressed images (recovery.bin.bz)

export RECOVERY_BIN_PATH="$1"
export RECOVERY_PARTNAME=Recovery


get_image() {
	local image="$1"
	local output_image="$2"
	
	local magic="$(dd if="$image" bs=2 count=1 2>/dev/null | hexdump -n 2 -e '1/1 "%02x"')"
	case "$magic" in
		1f8b) cmd="zcat";;
		425a) cmd="bzcat";;
		*) cmd="cat";;
	esac

	cat "$image" 2>/dev/null | $cmd > "$output_image"
}

check_image()
{
	local image=${1}
	
	local upgrade_possible=$(is_upgrade_possible $image)
	if [ $upgrade_possible = "no" ]; then
		echo "Upgrade is not possible, it would break a hub."
		exit 1
	fi
}

write_image() {
	local image="$1"
	local partname="$2"
	local uncompressed_image="/tmp/recovery_tmp.img"
	
	get_image "$image" "$uncompressed_image"
	check_image "$uncompressed_image"
	
	echo "Writing image to $partname..."
	mtd write "$uncompressed_image" "$partname"
}

update_recovery() {
	echo "Performing update of recovery partition..."
	write_image $RECOVERY_BIN_PATH $RECOVERY_PARTNAME
	[ $? -eq 0 ] && echo "Update completed" || echo "Failed to update"
}

[ $# -lt 1 ] && { echo "Specify an image to use for updating"; exit 1; }
update_recovery
