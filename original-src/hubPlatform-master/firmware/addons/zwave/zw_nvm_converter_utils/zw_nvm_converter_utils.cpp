#include "zwave/zw_nvm_converter_utils/zw_nvm_converter_utils.h"

#include "zwave/resources/zwave_device_info.h"
#include "log_utils/ezlo_log.h"

#include <fstream>
#include <cstdlib>
#include <cassert>
#include <unistd.h>

extern "C" {
#include "zw_nvm_converter/nvmconv.h"
}

namespace HubPlatform {
namespace Zwave {
namespace NvmConverterUtils {

namespace {

zw_nvm_format formatToStirng( Format format ) {
    switch( format ) {
    case Format::Static6_6:
        return ZW_NVM_FORMAT_STATIC_6_6;
    case Format::Static6_7:
        return ZW_NVM_FORMAT_STATIC_6_7;
    case Format::Static6_8:
        return ZW_NVM_FORMAT_STATIC_6_8;

    case Format::Bridge6_6:
        return ZW_NVM_FORMAT_BRIDGE_6_6;
    case Format::Bridge6_7:
        return ZW_NVM_FORMAT_BRIDGE_6_7;
    case Format::Bridge6_8:
        return ZW_NVM_FORMAT_BRIDGE_6_8;

    case Format::Bridge7_11:
        return ZW_NVM_FORMAT_BRIDGE_7_11;
    case Format::Bridge7_12:
        return ZW_NVM_FORMAT_BRIDGE_7_12;
    case Format::Bridge7_15:
        return ZW_NVM_FORMAT_BRIDGE_7_15;

    default:
        assert( false );
        return nullptr;
    }
}

}  // namespace

bool convertImage( Format inputFormat,
                   Format outputFormat,
                   const std::string& oldImagePath,
                   const std::string& newImagePath,
                   std::function< bool( Json& ) > jsonImageModifier ) {
    char jsonImageName[] = "/tmp/tmp.XXXXXX";
    close( mkstemp( jsonImageName ) );

    if( !zw_nvm_export( formatToStirng( inputFormat ), oldImagePath.c_str(), jsonImageName, false ) ) {
        LOGGER_ERROR( "Image export failed" );
        return false;
    }

    std::unique_ptr< const char, decltype( &remove ) > jsonRemover( jsonImageName, &remove );

    if( jsonImageModifier ) {
        std::string content;

        {
            std::ifstream stream( jsonImageName );
            content.assign( ( std::istreambuf_iterator< char >( stream ) ), std::istreambuf_iterator< char >() );
        }

        Json jsonFile;

        try {
            jsonFile = Json::parse( content );
        }
        catch( const std::exception& ) {
            LOGGER_ERROR( "Cannot parse temp json" );
            return false;
        }

        content.clear();

        if( jsonImageModifier( jsonFile ) ) {
            content = jsonFile.dump();

            std::ofstream stream( jsonImageName );
            stream << content;
        }
    }

    if( !zw_nvm_import( formatToStirng( outputFormat ), jsonImageName, newImagePath.c_str() ) ) {
        LOGGER_ERROR( "Image import failed" );
        return false;
    }

    LOGGER_INFO( "Image conversion done" );
    return true;
}

bool convertVeraImageToActualFormat( const std::string& oldImagePath,
                                     const std::string& newImagePath,
                                     std::function< void( const Json& ) > jsonHandler ) {
    constexpr const auto inputFormat = Format::Static6_7;
    constexpr const auto outputFormat = Format::Bridge7_12;

    return convertImage( inputFormat, outputFormat, oldImagePath, newImagePath, [&jsonHandler]( Json& json ) {
        const auto& locale = Resources::DeviceInfo::getZwaveChipLocale();

        json[ "appConfig" ][ "rfRegion" ] = locale == Resources::DeviceInfo::ChipLocaleUS ? 1 : 0;

        if( locale != Resources::DeviceInfo::ChipLocaleUS && locale != Resources::DeviceInfo::ChipLocaleEU ) {
            LOGGER_WARN( "Conversion locale " + locale + " to rfRegion could be not proper" );
        }

        if( jsonHandler ) {
            jsonHandler( json );
        }
        return true;
    } );
}

}  // namespace NvmConverterUtils
}  // namespace Zwave
}  // namespace HubPlatform
