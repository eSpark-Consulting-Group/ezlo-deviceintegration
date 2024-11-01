Original: SimplicityStudio_v5/developer/sdks/zwave/zip_gateway/v7.15.4/Source/zipgateway-7.15.04-Source.tar.bz2
Original Manual: SimplicityStudio_v5/developer/sdks/zwave/zip_gateway/v7.15.4/Doxygen/zipgateway-7.15.04-docs.zip/html/migration.html

-------------------------------------------------------------------

target: zw_nvm_converter

Converts zwave chip images of different versions to json file and vice versa.

Example of usage:
./zw_nvm_converter -e static6.6  nvm_old.bin  nvm.json
./zw_nvm_converter -i bridge7.12 nvm.json     nvm_new.bin

Supported formats are:

    bridge7.15 : NVM Converter for Z-Wave Bridge 7.15
    bridge7.12 : NVM Converter for Z-Wave Bridge 7.12
    bridge7.11 : NVM Converter for Z-Wave Bridge 7.11
    bridge6.8 : NVM Converter for Z-Wave Bridge 6.80
    bridge6.7 : NVM Converter for Z-Wave Bridge 6.70
    bridge6.6 : NVM Converter for Z-Wave Bridge 6.60
    static6.8 : NVM Converter for Z-Wave Static 6.80
    static6.7 : NVM Converter for Z-Wave Static 6.70
    static6.6 : NVM Converter for Z-Wave Static 6.60

-------------------------------------------------------------------

target: zw_nvm_converter_api.so
header: zw_nvm_converter/nvmconv.h

API:

bool zw_nvm_export(const char* format, const char* in_file, const char* out_file, bool pretty);

bool zw_nvm_import(const char* format, const char* in_file, const char* out_file);

