#include "scenes_manager/scenes_processor/parsers/scene_parser.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "database_client/dbqueryparser.h"
#include <memory>

namespace HubPlatform {
namespace ScenesManager {

namespace SceneFields {
constexpr char HouseModes[] = "house_modes";
constexpr char Name[] = "name";
constexpr char Parent_id[] = "parent_id";
constexpr char Enabled[] = "enabled";
constexpr char Global[] = "global";
constexpr char GroupId[] = "group_id";
constexpr char isGroup[] = "is_group";
constexpr char Tag[] = "tag";
constexpr char UserNotifications[] = "user_notifications";
constexpr char Icon[] = "icon";
constexpr char Generator[] = "generator";
}  // namespace SceneFields

SceneParser::SceneParser( CoreDRemoteInterface::CoreDInterface& core )
: m_core( core ) {}

SceneParser::ScenePtr SceneParser::create( const Json& sceneJsonObject ) {
    auto id = json_get_str( sceneJsonObject, db::ID_FIELD_NAME );
    return std::make_shared< SceneEngine::Scene >( std::move( id ) );
}

SceneParser::ScenePtr SceneParser::parse( const Json& sceneJsonObject, bool isEditing ) {
    if( sceneJsonObject.empty() ) {
        throw SceneEngine::SceneException( Errors::WRONG_PARAMS );
    }
    auto id = json_get_str( sceneJsonObject, db::ID_FIELD_NAME );
    if( id.size() > SceneIdMaxSize ) {
        auto reason = "_id field is more than " + std::to_string( SceneParserInterface::SceneIdMaxSize ) + " symbols";
        throw SceneEngine::SceneException( Errors::INVALID_VALUE_RANGE, db::ID_FIELD_NAME, reason.c_str() );
    }
    auto icon = json_get_str( sceneJsonObject, SceneFields::Icon );
    if( icon.size() > IconMaxSize ) {
        auto reason = "icon field is more than " + std::to_string( SceneParserInterface::IconMaxSize ) + " symbols";
        throw SceneEngine::SceneException( Errors::INVALID_VALUE_RANGE, SceneFields::Icon, reason.c_str() );
    }
    auto name = json_get_str( sceneJsonObject, SceneFields::Name );
    if( name.size() > NameMaxSize ) {
        auto reason = "name field is more than " + std::to_string( SceneParserInterface::NameMaxSize ) + " symbols";
        throw SceneEngine::SceneException( Errors::INVALID_VALUE_RANGE, SceneFields::Name, reason.c_str() );
    }
    Json generator;
    processGeneratorField( sceneJsonObject, generator, isEditing );

    ScenePtr scenePtr = std::make_shared< SceneEngine::Scene >( std::move( id ) );
    scenePtr->setName( name );
    scenePtr->setParentId( json_get_str( sceneJsonObject, SceneFields::Parent_id ) );
    scenePtr->setEnabled( json_get_bool( sceneJsonObject, SceneFields::Enabled ) );
    scenePtr->setGlobal( json_get_bool( sceneJsonObject, SceneFields::Global ) );
    scenePtr->setTag( json_get_str( sceneJsonObject, SceneFields::Tag ) );
    scenePtr->setGroupId( json_get_str( sceneJsonObject, SceneFields::GroupId ) );
    scenePtr->setGroupEnabled( json_get_bool( sceneJsonObject, SceneFields::isGroup ) );
    scenePtr->setIcon( icon );
    scenePtr->setGenerator( generator );
    setRoomName( *scenePtr );
    setUserNotifications( sceneJsonObject, *scenePtr );
    setHouseModes( sceneJsonObject, *scenePtr );

    return scenePtr;
}

bool SceneParser::setUserNotifications( const JsonObject& sceneJsonObject, SceneEngine::Scene& scene ) {
    JsonArray notifications = JSON_EMPTY_ARR;
    if( !json_get_arr( sceneJsonObject, SceneFields::UserNotifications, notifications ) ) {
        return false;
    }
    for( auto it = notifications.begin(); it != notifications.end(); ++it ) {
        std::string userId = json_get_str( *it );
        if( !userId.empty() ) {
            scene.addUserNotification( userId );
        }
    }
    return true;
}

bool SceneParser::setHouseModes( const JsonObject& sceneJsonObject, SceneEngine::Scene& scene ) {
    JsonArray houseModesArray = JSON_EMPTY_ARR;
    if( !json_get_arr( sceneJsonObject, SceneFields::HouseModes, houseModesArray ) ) {
        return false;
    }
    for( auto it = houseModesArray.begin(); it != houseModesArray.end(); ++it ) {
        if( it->is_string() ) {
            scene.addHouseMode( it->get< std::string >() );
        }
    }
    return true;
}

void SceneParser::setRoomName( SceneEngine::Scene& scene ) {
    JsonObject room = m_core.getRoom( scene.getParentId() );
    scene.setRoomName( json_get_str( room, "name" ) );
}

void SceneParser::processGeneratorField( const JsonObject& sceneJsonObject, JsonObject& generator, bool isEditing ) {
    if( json_contains( sceneJsonObject, SceneFields::Generator ) ) {
        json_get( sceneJsonObject, SceneFields::Generator, generator );
        if( !generator.is_object() ) {
            auto reason = "generator field is not an object field";
            throw SceneEngine::SceneException( Errors::WRONG_FIELD, SceneFields::Generator, reason );
        }
        else {
            try {
                jsonCheckString( generator, "name", true );
                jsonCheckString( generator, "version", true );
                jsonCheckString( generator, "role", true );
                auto currentTimestamp = std::chrono::system_clock::now();
                auto timeSeconds = std::chrono::system_clock::to_time_t( currentTimestamp );
                if( !isEditing ) {
                    json_add_int64( generator, "created", timeSeconds );
                }
                json_add_int64( generator, "modified", timeSeconds );
            }
            catch( const WrongParamsError& ex ) {
                auto reason = "is not a string field";
                throw SceneEngine::SceneException( Errors::WRONG_FIELD, ex.what(), reason );
            }
            catch( const WrongFieldError& ex ) {
                auto reason = "is not present";
                throw SceneEngine::SceneException( Errors::EMPTY_FIELD, ex.what(), reason );
            }
        }
    }
    else {
        auto currentTimestamp = std::chrono::system_clock::now();
        auto timeSeconds = std::chrono::system_clock::to_time_t( currentTimestamp );
        json_add_str( generator, "name", "unknown" );
        json_add_str( generator, "version", "" );
        json_add_str( generator, "role", "user" );
        json_add_int64( generator, "created", timeSeconds );
        json_add_int64( generator, "modified", timeSeconds );
    }
}

}  // namespace ScenesManager
}  // namespace HubPlatform
