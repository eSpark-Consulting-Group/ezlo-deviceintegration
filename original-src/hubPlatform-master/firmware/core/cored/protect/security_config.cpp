#include "security_config.h"
#include "resources.h"
#include "json_utils/json_wrapper.h"
#include "json_utils/json_utils.h"
#include "protect_utils.h"

namespace HubPlatform {
namespace CoreD {
namespace Protect {
using namespace Resources;
Json SecurityConfigInfo::toJson() const {
    JsonObject result = JSON_EMPTY_OBJ;
    result[ "_id" ] = m_id;
    result[ SecurityConfig::Name ] = m_name;
    result[ SecurityConfig::Description ] = m_description;
    result[ SecurityConfig::Alarm ] = m_alarm;
    result[ SecurityConfig::SoundType ] = m_soundType;
    result[ SecurityConfig::EntryDelay ] = m_entryDelay;
    result[ SecurityConfig::Chime ] = m_chime;
    result[ SecurityConfig::Follower ] = m_follower;
    result[ SecurityConfig::ArmedWhen ] = m_armedWhen;
    result[ SecurityConfig::RecommendCategories ] = m_recommendCategories;
    result[ SecurityConfig::Signal ] = m_signal;
    result[ SecurityConfig::Persistent ] = m_persistent;
    return result;
}

bool SecurityConfigInfo::fromJson( const JsonObject& data ) {
    JsonArray armedWhen, recommendedCategories;
    JsonObject signal;
    m_id = json_get_str( data, "_id" );
    m_name = json_get_str( data, SecurityConfig::Name );
    m_description = json_get_str( data, SecurityConfig::Description );
    m_alarm = json_get_bool( data, SecurityConfig::Alarm );
    m_soundType = json_get_str( data, SecurityConfig::SoundType );
    m_entryDelay = json_get_str( data, SecurityConfig::EntryDelay );
    m_chime = json_get_str( data, SecurityConfig::Chime );
    m_follower = json_get_str( data, SecurityConfig::Follower );
    json_get_arr( data, SecurityConfig::ArmedWhen, armedWhen );
    json_get_arr( data, SecurityConfig::RecommendCategories, recommendedCategories );
    json_get_obj( data, SecurityConfig::Signal, signal );
    m_armedWhen = armedWhen;
    m_recommendCategories = recommendedCategories;
    m_signal = signal;
    m_persistent = json_get_bool( data, SecurityConfig::Persistent, false );

    return !m_id.empty() && !m_name.empty() && !m_signal.empty();
}

bool SecurityConfigInfo::getAlarmFlag() const {
    return m_alarm;
}

std::string SecurityConfigInfo::getSoundType() const {
    return m_soundType;
}

JsonObject SecurityConfigInfo::getSignalObject() const {
    return m_signal;
}

std::string SecurityConfigInfo::getChime() const {
    return m_chime;
}

void SecurityConfigInfo::buildSignalField( const JsonObject& signal,
                                           const std::string& signalField,
                                           IDbClient& dbClient,
                                           Json& result ) {
    JsonObject field;
    std::pair< std::string, std::string > codeAliasPair;
    json_get_obj( signal, signalField, field );
    if( json_get_bool( field, "send", false ) ) {
        JsonObject event;
        if( json_get( field, "event", event ) ) {
            codeAliasPair.first = json_get_str( event, "code" );
            codeAliasPair.second = json_get_str( event, "alias" );
        }
    }

    if( !codeAliasPair.first.empty() && !codeAliasPair.second.empty() ) {
        const auto contactIdObject = Protect::ProtectUtils::getContactIdObject( dbClient, codeAliasPair.first );
        const auto alias = json_get_str( contactIdObject, codeAliasPair.second );
        result[ Protect::Resources::SecurityConfig::Signal ] = alias;
    }
}

JsonArray SecurityConfigInfo::getArmedWhen() const {
    return m_armedWhen;
}

std::string SecurityConfigInfo::getEntryDelay() const {
    return m_entryDelay;
}

}  // namespace Protect
}  // namespace CoreD
}  // namespace HubPlatform