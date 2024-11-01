# Utility to update ZWave SDK

## Utility functionality:
This utility is intended for updating the ZWave chip firmware (SDK) through the Serial API and is compatible only with hubs featuring the 700 Series ZWave chip. Check the ZWave gateway settings to determine the chip version of the HUB.

It requires two parameters: the location of the serial port and the location of the binary file containing the firmware.

NOTE: Do not use it while the firmware is running.

## Repository content:

|Source File|Description|
|-----------|-----------|
|CMakeLists.txt|CMake file|
|ha_718.hex|SDK version 7.18 in hex format, if the process failed (this version will be installed automatically)|
|ha_719.hex|SDK version 7.19 in hex format(only needed part), it is used for a hack to update from 7.17-7.18 versions to 7.20-7.2x version|
|ha_799.hex|SDK version 7.99 in hex format(not real version), it is used for a hack to downgrade SDK version|
|main.cpp|Source code|
|main.h|Global variables|
|README.h|Readme|

## Utility usage

### Help
```
./zw_programmer -h
```

### Update SDK
```
./zw_programmer /dev/ttyUSB0 z14_7_20_2.gbl
```

Where first param is port to zwave chip, second is address to file with SDK

## Utility execution result

|Code|Description|
|----|-----------|
|0   |Finished successfully|
|-1  |Port open failed|
|-2  |File open failed|
|-3  |Problem with bootloader|
|-4  |Problem with XModem|
|-5  |SDK 7.18 is installed, due to internal error|
|-6  |Invalid arguments passed to the utility|
