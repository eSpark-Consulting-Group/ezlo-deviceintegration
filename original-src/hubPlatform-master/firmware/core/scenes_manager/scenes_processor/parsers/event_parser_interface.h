#pragma once

#include "scenes_manager/scenes_processor/id_utils.h"
#include "scenes_manager/scenes_engine/events/item_value_provider.h"
#include "scenes_manager/scenes_engine/value_source_key.h"
#include "cored/remote_interface/system_attribute_types.h"
#include "scenes_manager/scenes_engine/scene.h"

namespace SceneEngine {
class IClock;
class EventContainer;
}  // namespace SceneEngine

namespace HubPlatform {

namespace Services {
class TaskLoop;
}

namespace ScenesManager {

class EventParserInterface {
public:
    using ControllerId = std::string;
    using GetGeoPositionCallback = std::function< HubPlatform::CoreDRemoteInterface::GeoPosition() >;
    using ClockGetter = std::function< SceneEngine::IClock&() >;
    using MakeGenericValueProvider = std::function< SceneEngine::ItemValueProviderPtr( SceneEngine::ValueSourceKey ) >;

public:
    virtual ~EventParserInterface() = default;
    virtual SceneEngine::EventContainer parseTriggers( Json& sceneJsonObject,
                                                       IdUtils::PlaceholdersMapping& idsMapping ) = 0;
    virtual SceneEngine::EventContainer parseExceptions( Json& sceneJsonObject,
                                                         IdUtils::PlaceholdersMapping& idsMapping ) = 0;
    virtual SceneEngine::EventContainer parseGlobalRestriction( SceneEngine::SceneSharedPtr scenePtr,
                                                                Json& sceneJsonObject,
                                                                IdUtils::PlaceholdersMapping& idsMapping ) = 0;
    virtual void setClockGetter( ClockGetter clockGetter ) = 0;
};

class EventParserFactory {
public:
    static std::unique_ptr< EventParserInterface > makeEventParser(
        EventParserInterface::ControllerId controllerId,
        EventParserInterface::GetGeoPositionCallback getGeoPosition,
        EventParserInterface::ClockGetter clockGetter,
        EventParserInterface::MakeGenericValueProvider makeGenericValueProvider,
        Services::TaskLoop& taskLoop );
};

}  // namespace ScenesManager
}  // namespace HubPlatform
