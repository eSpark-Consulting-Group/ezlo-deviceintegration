to create serial API compatible with fb-zwflash utility you need:
- fresh hex file from Sigma Designes ( Silabs ) - serialapi_controller_static_ZW050x_US.hex - SDK version 6.81.01 or higher ( 6.81.03 works as expected )
- create binary - invoke objcopy with next parameters:
	/usr/local/toolchains/gcc7.2-arm/bin/arm-linux-gnueabihf-objcopy -I ihex --output-target=binary --gap-fill 0xFF --pad-to 0x20000 serialapi_controller_static_ZW050x_US_v6_81_03.hex US_flash_v6_81_03.bin
- move binary to the hub folder ( /opt/firmware/zw-flash ) and stop daemons
- run fb-zwflash utility from /opt/firmware/bin with "-f" parameter like:
	/opt/firmware/bin/fb-zlash -f

