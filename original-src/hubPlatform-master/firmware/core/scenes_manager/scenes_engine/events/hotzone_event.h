#pragma once

#include "scenes_manager/scenes_engine/events/event_base.h"

#include "cored/model/values/hotzone_match_value.h"
#include "variant/variant.hpp"

#include <set>

namespace SceneEngine {

namespace UT {
class HotzoneEventTest;
};

class HotzoneEvent : public EventBase {
public:
    using Hotzone = std::string;
    using Hotzones = std::set< Hotzone >;
    using DetectableType = std::string;
    using Filter = HubPlatform::CoreD::Model::HotzoneMatchValue;
    using Filters = std::map< DetectableType, Filter >;
    enum class MatchPolicy { Any, None, Filter };

    using HotzonesItemValue = JsonObject;

    HotzoneEvent( Hotzones hotzones, MatchPolicy matchPolicy, Filters filters );

public:
    std::string getMethodName() const override;
    bool compare( const HotzonesItemValue& item );

private:
    bool updateTrigger( bool newCompareResult );

public:
    static bool isDetectableType( const DetectableType& type );

private:
    Hotzones m_hotzones;
    MatchPolicy m_matchPolicy = MatchPolicy::Any;
    Filters m_filters;
    bool m_isTriggered = false;

    friend class UT::HotzoneEventTest;
};

using HotzoneEventPtr = std::shared_ptr< HotzoneEvent >;

}  // namespace SceneEngine
