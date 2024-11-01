#include "cored/features_manager.h"
#include "cored/model/resources/cored_resources.h"
#include "scenes_manager/scenes_engine/advanced_scenes_version.h"

#include "base_utils/semantic_version.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"
#include "system_utils/filesystem.h"
#include "system_utils/environment.h"

#include "service_interfaces/service.h"
#include "cored/script_events/feature_added.h"
#include "cored/script_events/feature_removed.h"
#include "cored/script_events/feature_updated.h"
#include "metrics/reporting_schema_version.h"

#include <utility>
#include <vector>
#include <fstream>

namespace HubPlatform {
namespace CoreD {

#if defined( __HUB_WITH_WEBRTC__ )
#define WITH_WEBRTC true
#else
#define WITH_WEBRTC false
#endif

IFeaturesManager::~IFeaturesManager() = default;

FeaturesManager::FeaturesManager( IDbClient& dbClient )
: m_dbClient( dbClient ) {
    using namespace Model::Resources;

    const bool isCustom = true;

    static std::vector< ControllerFeature > ControllerFeatures = {
        { "local_mode", { 1, 0 }, true, false, !isCustom },
        { "advanced_scenes",
          { FEATURE_ADVANCED_SCENES_VERSION_MAJOR, FEATURE_ADVANCED_SCENES_VERSION_MINOR },
          true,
          false,
          !isCustom },
        { "cloud_devices", { 1, 0 }, true, false, !isCustom },
        { "cameras", { 1, 0 }, true, false, !isCustom },
        { "device_settings", { 1, 0 }, true, false, !isCustom },
        { "backups", { 1, 0 }, false, false, !isCustom },
        { "cloud_storage", { 2, 0 }, true, false, !isCustom },
        { "webrtc", { 1, 0 }, WITH_WEBRTC, false, !isCustom },
#if !defined( __HUB_TYPE_G150__ )
        { "mqtt_gateway", { 1, 0 }, true, false, !isCustom },
#endif
#if defined( __HUB_TYPE_H2_SECURE__ ) || defined( __HUB_TYPE_H2__ ) || defined( __HUB_TYPE_LOCAL__ )
        { "protect", { 2, 0 }, true, true, !isCustom },
        { "protect_delayed", { 1, 0 }, true, false, !isCustom },
        { "proactive_reporting", { 1, PROACTIVE_REPORTING_PROTOBUF_SCHEMA_VERSION }, true, false, !isCustom },
#elif defined( __HUB_TYPE_VHUB__ )
    // "protect" is not available
#elif defined( __HUB_TYPE_BEEWAVE__ )
    // "protect" is not available
#elif defined( __HUB_TYPE_G150__ )
    // "protect" is not available
#else
#error "Please specify controller features for current controller type"
#endif
    };

    for( auto& feature : ControllerFeatures ) {
        auto featurePresent = m_dbClient.has( DBNames::FeaturesDB, JSON( db::ID_FIELD_NAME << feature.name ) );
        if( !featurePresent ) {
            m_dbClient.insert( DBNames::FeaturesDB, feature.toDbRecord() );
        }
        else {
            m_dbClient.update( DBNames::FeaturesDB,
                               JSON( db::ID_FIELD_NAME << feature.name ),
                               JSON( "$set" << feature.toDbRecordBody() ) );

            if( feature.name == "webrtc" ) {
                // editable -> !editable, status is taken from macro, cannot be set from outside. Reset status if needed
                fixWebRTCFeature( feature );
            }
        }
    }

    auto featuresArray = m_dbClient.findAll( DBNames::FeaturesDB, JSON( FeaturesFields::IsCustom << false ), nullptr );

    for( int i = 0; i < featuresArray.size(); i++ ) {
        bool isPresent = false;
        for( auto& feature : ControllerFeatures ) {
            if( featuresArray[ i ][ db::ID_FIELD_NAME ] == feature.name ) {
                isPresent = true;
                break;
            }
        }
        if( !isPresent ) {
            m_dbClient.remove( DBNames::FeaturesDB,
                               JSON( db::ID_FIELD_NAME << featuresArray[ i ][ db::ID_FIELD_NAME ] ) );
        }
    }
}

void FeaturesManager::setFeatureStatus( const std::string& feature, bool status ) {
    using namespace Model::Resources;

    bool previousStatus = getFeatureStatus( feature );
    m_dbClient.update( DBNames::FeaturesDB,
                       JSON( db::ID_FIELD_NAME << feature ),
                       JSON( "$set" << JSON( FeaturesFields::Status << status ) ) );

    if( previousStatus != status ) {
        emitStatusUpdated( feature, status );
    }
}

bool FeaturesManager::getFeatureStatus( const std::string& feature ) const {
    using namespace Model::Resources;

    const auto featureJs = m_dbClient.findOne( DBNames::FeaturesDB, JSON( db::ID_FIELD_NAME << feature ), nullptr );
    return !featureJs.empty() && json_get_bool( featureJs, FeaturesFields::Status, false );
}

void FeaturesManager::onFeatureStatusUpdated( Callback callback ) {
    m_onStatusUpdated = std::move( callback );
}

/** Feature status for 'webrtc' feature can change relatively to compilation options. Update it in database accordingly.
 */
void FeaturesManager::fixWebRTCFeature( const ControllerFeature& feature ) {
    setFeatureStatus( feature.name, feature.status );
}

void FeaturesManager::emitStatusUpdated( const std::string& feature, bool status ) {
    if( m_onStatusUpdated != nullptr ) {
        m_onStatusUpdated( feature, status );
    }
}

ExternalFeaturesManager::ExternalFeaturesManager( Services::Service& service )
: FeaturesManager( service.getDBClient() )
, m_service( service ) {
    onFeatureStatusUpdated( [this]( const std::string& feature, bool status ) {
        LOGGER_DEBUG_RAW( "Emit feature status events: feature: " + feature + ", status: " + cc::flag( status ) );

        using namespace Model::Resources;
        emitEventUpdated( JSON( FeaturesFields::Id << feature << FeaturesFields::Status << status ) );
    } );
}

void ExternalFeaturesManager::setUpExternalFeatures() {
    LOGGER_INFO( cc::info( "Feature Manager: Entering external feature setup" ) );
    using namespace Model::Resources;
    auto externalPackagesFiles = getCustomPackagesFiles();
    if( !externalPackagesFiles.empty() ) {
        for( auto& singlePackageFile : externalPackagesFiles ) {
            LOGGER_DEBUG( cc::debug( "Start adding external feature from file: " ) + cc::j( singlePackageFile ) );
            JsonObject content;
            auto filePath = Environment::getCustomPackagesFolder() + "/" + singlePackageFile;
            try {
                std::ifstream file( filePath );
                file >> content;
            }
            catch( const std::exception& ex ) {
                Log::error( cc::error( "Error loading json file: " ) + ex.what() );
                continue;
            }
            LOGGER_DEBUG( cc::debug( "file data = " ) + cc::j( content ) );
            processPackageAdding( content );
        }
    }

    auto featuresArray = m_dbClient.findAll( CoreFeaturesDBName, JSON( FeaturesFields::IsCustom << true ), nullptr );

    LOGGER_DEBUG( cc::debug( "Start checking custom feature to remove from list" ) );
    for( auto& feature : featuresArray ) {
        LOGGER_DEBUG( cc::debug( "checking custom feature: " ) + cc::j( feature ) );
        auto packageName = json_get_str( feature, db::ID_FIELD_NAME );
        bool isPresent = false;
        for( auto& singlePackageFile : externalPackagesFiles ) {
            JsonObject content;
            auto filePath = Environment::getCustomPackagesFolder() + "/" + singlePackageFile;
            LOGGER_DEBUG( cc::debug( "Check removing of external feature from file: " ) + cc::j( singlePackageFile ) );
            try {
                std::ifstream file( filePath );
                file >> content;
            }
            catch( const std::exception& ex ) {
                Log::error( cc::error( "Error loading json file: " ) + ex.what() );
                continue;
            }
            isPresent = processPackageRemoving( content, packageName );
            if( isPresent ) {
                break;
            }
        }
        if( !isPresent ) {
            removeCustomFeature( packageName );
        }
    }
    LOGGER_INFO( cc::info( "Feature Manager: Exiting external feature setup" ) );
}

std::vector< std::string > ExternalFeaturesManager::getCustomPackagesFiles() {
    std::vector< std::string > files;
    if( filesystem::isDirectoryExist( Environment::getCustomPackagesFolder() ) ) {
        files = filesystem::findAllFiles( Environment::getCustomPackagesFolder() );
    }
    LOGGER_INFO( cc::info( "Total files present in custom_packages directory are: " ) + cc::num10( files.size() ) );
    return files;
}

bool ExternalFeaturesManager::processPackageRemoving( const JsonObject& fileData, std::string& packageName ) {
    using namespace Model::Resources;
    auto isPresent = false;
    JsonArray customFeatureArray;
    auto service = json_get_str( fileData, "service" );
    json_get_arr( fileData, "custom_features", customFeatureArray );
    for( auto& feature : customFeatureArray ) {
        auto suffixName = json_get_str( feature, FeaturesFields::Name );
        auto package = service + "." + suffixName;
        if( packageName == package ) {
            isPresent = true;
            break;
        }
    }
    return isPresent;
}

void ExternalFeaturesManager::processPackageAdding( const JsonObject& fileData ) {
    using namespace Model::Resources;

    JsonArray customFeatureArray;
    auto service = json_get_str( fileData, "service" );  // TODO: rename service to package
    if( !json_get_arr( fileData, "custom_features", customFeatureArray ) ) {
        Log::error( "Custom features not found" );
        return;
    }
    if( service.empty() ) {
        Log::error( "Custom feature required field not found: service" );
        return;
    }
    for( const auto& featureJson : customFeatureArray ) {
        ControllerFeature feature;
        try {
            feature.fromJson( service, featureJson );
        }
        catch( std::exception& ex ) {
            Log::error( "Feature parsing failed for feature: " + cc::j( featureJson ) + ", error: " + ex.what() );
            continue;
        }

        const bool isCustom = true;
        auto featurePresent = m_dbClient.has(
            DBNames::FeaturesDB, JSON( db::ID_FIELD_NAME << feature.name << FeaturesFields::IsCustom << isCustom ) );
        if( !featurePresent ) {
            addCustomFeature( feature );
        }
        else {
            updateCustomFeature( feature );
        }
    }
}

void ExternalFeaturesManager::addCustomFeature( const ControllerFeature& feature ) {
    LOGGER_DEBUG( cc::debug( "Adding custom feature: " ) + cc::j( feature.toDbRecord() ) );
    using namespace Model::Resources;
    m_dbClient.insert( DBNames::FeaturesDB, feature.toDbRecord() );
    emitEventAdded( JSON( feature.toDbRecord() ) );
}

void ExternalFeaturesManager::updateCustomFeature( const ControllerFeature& feature ) {
    LOGGER_DEBUG( cc::debug( "Updating custom feature: " ) + cc::j( feature.toDbRecord() ) );
    using namespace Model::Resources;

    auto name = feature.name;
    const auto record = m_dbClient.findOne( DBNames::FeaturesDB, JSON( db::ID_FIELD_NAME << name ), nullptr );
    ControllerFeature currentFeature;
    currentFeature.fromRecordJson( name, record );
    Json broadcastResult = jsonDiff( currentFeature.toDbRecord(), feature.toDbRecord(), true );

    if( !broadcastResult.empty() ) {
        json_add_str( broadcastResult, db::ID_FIELD_NAME, feature.name );
        m_dbClient.update( DBNames::FeaturesDB,
                           JSON( db::ID_FIELD_NAME << feature.name ),
                           JSON( "$set" << feature.toDbRecordBody() ) );
        emitEventUpdated( broadcastResult );
    }
}

void ExternalFeaturesManager::removeCustomFeature( const std::string& packageName ) {
    LOGGER_DEBUG( cc::debug( "Removing custom feature: " ) + packageName );
    using namespace Model::Resources;
    m_dbClient.remove( CoreFeaturesDBName, JSON( db::ID_FIELD_NAME << packageName ) );
    emitEventRemoved( JSON( db::ID_FIELD_NAME << packageName ) );
}

void ExternalFeaturesManager::emitEventUpdated( const JsonObject& payload ) const {
    m_service.sendBroadcast( HUB_FEATURE_UPDATED, payload, GroupsNames::BROADCAST_GROUP );

    using namespace Model::Resources;

    auto name = json_get_str( payload, FeaturesFields::Id );
    auto status = json_get_bool( payload, FeaturesFields::Status ) ? "on" : "off";
    m_service.emit( ScriptEvents::FeatureUpdated( JSON_EMPTY_OBJ, name, status ) );
}

void ExternalFeaturesManager::emitEventAdded( const JsonObject& payload ) const {
    m_service.sendBroadcast( HUB_FEATURE_ADDED, payload, GroupsNames::BROADCAST_GROUP );

    using namespace Model::Resources;

    auto name = json_get_str( payload, FeaturesFields::Id );
    auto status = json_get_bool( payload, FeaturesFields::Status ) ? "on" : "off";
    m_service.emit( ScriptEvents::FeatureAdded( JSON_EMPTY_OBJ, name, status ) );
}

void ExternalFeaturesManager::emitEventRemoved( const JsonObject& payload ) const {
    m_service.sendBroadcast( HUB_FEATURE_REMOVED, payload, GroupsNames::BROADCAST_GROUP );

    using namespace Model::Resources;

    auto name = json_get_str( payload, FeaturesFields::Id );
    m_service.emit( ScriptEvents::FeatureRemoved( JSON_EMPTY_OBJ, name ) );
}

bool ControllerFeature::fromJson( const std::string& baseName, const JsonObject& feature ) {
    checkFeature( feature );

    using namespace Model::Resources;
    auto nameSuffix = json_get_str( feature, FeaturesFields::Name );
    auto status = ( json_get_str( feature, FeaturesFields::Status ) == FeatureStatus::On );
    auto editable = json_get_bool( feature, FeaturesFields::Editable, false );
    auto featureVersion = json_get_str( feature, FeaturesFields::Version );
    auto featureName = nameSuffix;
    if( !baseName.empty() ) {
        featureName = baseName + "." + nameSuffix;
    }

    this->name = featureName;
    this->editable = editable;
    this->isCustom = true;
    this->status = status;
    Common::SemanticVersion< int > semver( featureVersion );
    this->version = semver;

    return true;
}

void ControllerFeature::fromRecordJson( const std::string& name, const JsonObject& feature ) {
    using namespace Model::Resources;
    status = ( json_get_str( feature, FeaturesFields::Status ) == FeatureStatus::On );
    editable = json_get_bool( feature, FeaturesFields::Editable, false );
    featureVersionStr = json_get_str( feature, FeaturesFields::Version );
}

// Check all fields, throw invalid_arg based error
void ControllerFeature::checkFeature( const JsonObject& json ) {
    using namespace Model::Resources;

    try {
        jsonCheckBool( json, FeaturesFields::Editable );
        jsonCheckString( json, FeaturesFields::Name );
        jsonCheckString( json, FeaturesFields::Status );
        auto status = json_get_str( json, FeaturesFields::Status );
        if( status != FeatureStatus::On && status != FeatureStatus::Off ) {
            throw ControllerFeatureCheckError( "Field 'status' invalid value" );
        }
        jsonCheckString( json, FeaturesFields::Version );

        auto version = json_get_str( json, FeaturesFields::Version );
        Common::SemanticVersion< int > semver( version );
        if( !semver.isValid() ) {
            throw ControllerFeatureCheckError( "Field 'version' invalid value" );
        }
    }
    catch( const std::runtime_error& ex ) {
        throw ControllerFeatureCheckError( std::string{ "Field check failed: " } + ex.what() );
    }
}

ControllerFeature::ControllerFeature( std::string name,
                                      SemanticVersion version,
                                      bool status,
                                      bool editable,
                                      bool isCustom )
: name( std::move( name ) )
, version( version )
, status( status )
, editable( editable )
, isCustom( isCustom ) {}

JsonObject ControllerFeature::toDbRecord() const {
    auto record = toDbRecordBody();
    using namespace Model::Resources;

    json_add_str( record, db::ID_FIELD_NAME, name );
    return record;
}

JsonObject ControllerFeature::toDbRecordBody() const {
    using namespace Model::Resources;
    return JSON( FeaturesFields::Version << version.toString() << FeaturesFields::Status << status
                                         << FeaturesFields::Editable << editable << FeaturesFields::IsCustom
                                         << isCustom );
}

std::string ControllerFeature::getVersion() {
    return version.toString();
}

bool ControllerFeature::getStatus() {
    return status;
}

bool ControllerFeature::getEditable() {
    return editable;
}

}  // namespace CoreD
}  // namespace HubPlatform
