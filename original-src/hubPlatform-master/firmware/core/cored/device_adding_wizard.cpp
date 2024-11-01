#include "device_adding_wizard.h"
#include "json_utils/json_wrapper.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"
#include <fstream>

DeviceAddingWizard::DeviceAddingWizard( const std::string& filePath )
: m_filePath( filePath ) {}

DeviceAddingWizard::Categories DeviceAddingWizard::getCategories() const {
    std::pair< JsonArray, bool > data = getFileData();
    if( !data.second ) {
        return data;
    }
    JsonArrayBuilder names;
    for( JsonObject& category : data.first ) {
        std::string name = getFieldSafe< std::string >( category, "category" );
        std::string languageTag = getFieldSafe< std::string >( category, "languageTag" );
        std::string iconTag = getFieldSafe< std::string >( category, "iconTag" );
        names << JSON( "category" << name << "languageTag" << languageTag << "iconTag" << iconTag );
    }
    return { JSON( "result" << std::move( names ) ), true };
}

DeviceAddingWizard::Devices DeviceAddingWizard::getDevices( const std::string& categoryFilter ) const {
    std::pair< JsonArray, bool > data = getFileData();
    if( !data.second ) {
        return data;
    }
    JsonArrayBuilder result;
    for( JsonObject& category : data.first ) {
        std::string categoryName = getFieldSafe< std::string >( category, "category" );
        if( !categoryFilter.empty() && categoryFilter != categoryName ) {
            continue;
        }
        JsonArray devices;
        if( !json_get( category, "devices", devices ) )
            return { JSON( Errors::ERROR_FIELD << JSON( "message"
                                                        << "invalid json" ) ),
                     false };
        for( JsonObject& device : devices ) {
            device[ "wizardCategory" ] = std::move( categoryName );
            result << std::move( device );
        }
    }
    return { JSON( "result" << std::move( result ) ), true };
}

std::pair< JsonArray, bool > DeviceAddingWizard::getFileData() const {
    std::ifstream jsonFile( m_filePath );
    if( !jsonFile.good() ) {
        return { JSON( Errors::ERROR_FIELD << JSON( "message"
                                                    << "source json file isn't opened" ) ),
                 false };
    }
    JsonObject json;
    JsonArray categories;
    try {
        json = JsonObject::parse( jsonFile );
    }
    catch( ... ) {
        return { JSON( Errors::ERROR_FIELD << JSON( "message"
                                                    << "invalid json" ) ),
                 false };
    }
    if( !json_get_arr( json, "categories", categories ) ) {
        return { JSON( Errors::ERROR_FIELD << JSON( "message"
                                                    << "invalid json" ) ),
                 false };
    }
    return { std::move( categories ), true };
}
