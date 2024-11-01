#include "uid/handlers/hub_features_commands_handler.h"
#include "uid/ui_service.h"

namespace HubPlatform {
namespace Uid {

HubFeaturesCommandsHandler::HubFeaturesCommandsHandler( Uid::Service& hostService )
: m_service( hostService )
, m_core( hostService ) {}

//  "params": {}
void HubFeaturesCommandsHandler::handleHubFeaturesList( rpc::Message& rpcMessage ) const {
    JsonObject& joAnswer = rpcMessage.getAnswer();
    joAnswer[ "result" ][ "features" ] = getFeaturesList();
}

//  "params": {
//      "feature": "protect",
//      "status": "off"
//  }
void HubFeaturesCommandsHandler::handleHubFeatureStatusSet( rpc::Message& rpcMessage ) {
    const JsonObject& params = rpcMessage.getParams();
    JsonObject& joAnswer = rpcMessage.getAnswer();

    std::string featureName;
    std::string statusString;

    if( !getNonEmptyField( params, "feature", joAnswer, featureName, false ) ) {
        return;
    }

    if( !getNonEmptyField( params, "status", joAnswer, statusString, false ) ) {
        return;
    }

    bool status;
    if( statusString == "on" ) {
        status = true;
    }
    else if( statusString == "off" ) {
        status = false;
    }
    else {
        JsonObject error;
        Errors::getInvalidValueRange( "status", error, "" );
        setErrorObject( joAnswer, error );
        return;
    }

    auto feature = m_core.findFeature( featureName );
    if( !feature ) {
        setError( joAnswer, Errors::OBJECT_NOT_FOUND, "Feature not found" );
        return;
    }

    auto oldStatus = feature->getStatus();
    if( status != oldStatus ) {
        if( !feature->setStatus( status ) ) {
            setError( joAnswer, Errors::UNEXPECTED_REQUEST, "Feature status can not be changed" );
            return;
        }

        m_core.setFeatureStatus( *feature );
    }
}

void HubFeaturesCommandsHandler::emitEvent( MessageSubClasses msgClass, const JsonObject& payload ) const {
    m_service.sendBroadcast( msgClass, payload, GroupsNames::BROADCAST_GROUP );
}

Json HubFeaturesCommandsHandler::getFeaturesList() const {
    Json result;
    auto features = m_core.getFeatures();
    for( auto& feature : features ) {
        result[ feature.getName() ] =
            JSON( "status" << ( feature.getStatus() ? "on" : "off" ) << "version" << feature.getVersion().toString() );
    }
    return result;
}

}  // namespace Uid
}  // namespace HubPlatform
