Description
==============

This manual describes how to setup H2 hub to update other controllers via ethernet cable.
The update mechanism is fully automated.

See the full description here:

    https://confluence.mios.com/display/ELFD/How+to+setup+H2+hub+to+update+other+controllers

How to build and pack the service
==============

Run the `create_release.sh` script

    ./create_release.sh

This will create a tar archieve in the `releases` directory

    ls releases/*.tgz

How to set up the firmware updater
==============

* Download archive with updater service to the controller into `/home` directory
* Unpack the archive and run the installer script

    cd /home && tar xvzf remote_updates_2024.10.09.tgz
    /home/remote_updates/install.sh

* Copy/download firmware file to `/home/remote_updates/files/firmware.img`. Next example downloads ezlo secure firmware `183`

    wget -O /home/remote_updates/files/firmware.img http://dl.mios.com/linux_firmware/bundle/h2_secure.1/v1/snapshots/183/ezlo-openwrt-sunxi-cortexa7-sun8i-h2-plus-orangepi-zero-squashfs-sdcard.img.gz

* Copy "controllers.bin" file (see confluence page)
* Reboot

