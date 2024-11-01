#!/bin/sh

if [ $# -lt 1 ]; then
	echo "specify an image to upgrade a boottrap"
	echo -e "usage:\n$0 g150_boottrap.bin"
	exit 1
fi

eb_size=$(( 128 * 1024 ))
bt_part=/dev/$(cat /proc/mtd | awk '/"Boottrap"/ {print substr( $1, 1, length($1) - 1 )}')
bt_part_size=$(( $(mtdinfo ${bt_part} | grep "Amount of eraseblocks" | awk '{print $4}') * ${eb_size} ))
bt_bin=${1}

check_image()
{
local image_bin=${1}
local uimage_magic="56190527"

local magic="$(dd if=${1} bs=4 count=1 2>/dev/null | hexdump -e '"%x"')"
case "$magic" in
	$uimage_magic) return 0;;
	*)  echo "the specified image is incorrect"; exit 1;;
esac
}

check_image ${bt_bin}

echo "Performing updating of boottrap... "
dd if=${bt_bin} ibs=${bt_part_size} count=1 | mtd write - ${bt_part}
if [ $? -ne 0 ]; then
	echo "Failed to update boottrap"
	exit 1
else
	echo "Update completed"
	exit 0
fi
