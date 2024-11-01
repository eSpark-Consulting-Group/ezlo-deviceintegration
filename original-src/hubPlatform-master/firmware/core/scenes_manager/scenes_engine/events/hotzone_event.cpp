#include "scenes_manager/scenes_engine/events/hotzone_event.h"
#include "common_utils/cpp14.h"
#include "tiny/tiny_consolecolors.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "scenes_manager/scenes_engine/events/event_exception.h"

#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/camera_hotzone_value.h"
#include "cored/model/values/value_factory.h"

#include <deque>
#include <algorithm>
#include <iterator>

namespace SceneEngine {

namespace {
// NOTE: hotzoneObjects is a dictionary: <DetectableObjectType,ArrayOfDetectedIds>
bool applyFiltersToHotzoneObjects( const HotzoneEvent::Filters& filters, const JsonObject& hotzoneObjects ) {
    using MatchPolicy = HotzoneEvent::Filter::MatchPolicy;
    size_t matches = 0;
    for( auto& filter : filters ) {
        auto& objectType = filter.first;
        auto& objectFilter = filter.second;
        const auto filterPolicy = objectFilter.getMatchPolicy();

        auto objectsIt = hotzoneObjects.find( objectType );
        bool objectsNotDetected = objectsIt == hotzoneObjects.cend() || objectsIt->empty();

        if( filterPolicy == MatchPolicy::None || filterPolicy == MatchPolicy::NoneOf ) {
            if( objectsNotDetected ) {
                ++matches;
                continue;
            }
            return false;
        }

        if( objectsNotDetected )  // later checks require non empty list of ids
            return false;

        if( filterPolicy == MatchPolicy::Any ) {
            ++matches;
            continue;
        }

        std::set< std::string > detectedIds;
        for( auto& jsonId : objectsIt.value() ) {
            detectedIds.emplace( jsonId.get< std::string >() );
        }

        std::set< std::string > intersection;

        auto& matchList = objectFilter.getMatchList();
        std::set_intersection( matchList.cbegin(),
                               matchList.cend(),
                               detectedIds.begin(),
                               detectedIds.end(),
                               std::inserter( intersection, intersection.begin() ) );

        if( filterPolicy == MatchPolicy::AnyOf ) {
            if( !intersection.empty() ) {
                ++matches;
                continue;
            }
            return false;
        }

        if( filterPolicy == MatchPolicy::AllOf ) {
            if( intersection.size() == matchList.size() ) {
                ++matches;
                continue;
            }
            return false;
        }

        if( filterPolicy == MatchPolicy::OneOf ) {
            if( intersection.size() == 1 ) {
                ++matches;
                continue;
            }
            return false;
        }

        if( filterPolicy == MatchPolicy::NoneOf ) {
            if( intersection.empty() ) {
                ++matches;
                continue;
            }
            return false;
        }
    }
    return matches == filters.size();
}
}  // namespace

bool HotzoneEvent::compare( const HotzonesItemValue& itemValue ) {
    namespace HotzoneFields = HubPlatform::CoreD::Model::Resources::CameraHotzoneValueFields;

    try {
        using namespace HubPlatform::CoreD::Model;
        auto valueType = ValueType::makeDictionaryType( ValueType::Enum::CameraHotzone );
        auto value = ValueFactory::createItemValue( valueType, itemValue );
    }
    catch( ... ) {
        throw EventException( "HotzoneEvent comparator got unknown type to compare" );
    }

    std::deque< const JsonObject* > hotzonesToCheck;
    if( !m_hotzones.empty() ) {
        // search in selected hotzones
        for( auto& hotzone : m_hotzones ) {
            auto it = itemValue.find( hotzone );
            if( it != itemValue.cend() )
                hotzonesToCheck.push_back( &( it.value() ) );
        }
        if( hotzonesToCheck.empty() )  // selected hotzones not found
            return updateTrigger( false );
    }
    else {
        // search in all hotzones
        for( auto it = itemValue.cbegin(); it != itemValue.cend(); ++it ) {
            hotzonesToCheck.push_back( &( it.value() ) );
        }
    }

    if( m_matchPolicy == MatchPolicy::Any ) {
        for( auto& hotzone : hotzonesToCheck ) {
            auto& detected = hotzone->at( HotzoneFields::Detected );
            for( auto& detectedObjects : detected ) {
                if( !detectedObjects.empty() )
                    return updateTrigger( true );
            }
        }
        return updateTrigger( false );
    }
    else if( m_matchPolicy == MatchPolicy::None ) {
        bool noDetectedObjects = true;
        for( auto& hotzone : hotzonesToCheck ) {
            auto& detected = hotzone->at( HotzoneFields::Detected );
            for( auto& detectedObjects : detected ) {
                if( !detectedObjects.empty() )
                    noDetectedObjects = false;
            }
        }
        return updateTrigger( noDetectedObjects );
    }
    else if( m_matchPolicy == MatchPolicy::Filter ) {
        if( m_filters.empty() )
            throw EventException( "Number of hotzone filters must be greater that zero" );

        for( auto& hotzone : hotzonesToCheck ) {
            auto& hotzoneObjects = hotzone->at( HotzoneFields::Detected );

            if( applyFiltersToHotzoneObjects( m_filters, hotzoneObjects ) )
                return updateTrigger( true );
        }
        return updateTrigger( false );
    }

    throw EventException( "HotzoneEvent comparator hits non supported execution branch" );
}

HotzoneEvent::HotzoneEvent( Hotzones hotzones, MatchPolicy matchPolicy, Filters filters )
: m_hotzones( std::move( hotzones ) )
, m_matchPolicy( matchPolicy )
, m_filters( std::move( filters ) ) {}

std::string HotzoneEvent::getMethodName() const {
    return "isDetectedInHotzone";
}

bool HotzoneEvent::updateTrigger( bool newCompareResult ) {
    const bool result = newCompareResult && !m_isTriggered;
    m_isTriggered = newCompareResult;
    return result;
}

bool HotzoneEvent::isDetectableType( const DetectableType& type ) {
    return HubPlatform::CoreD::Model::CameraHotzoneValue::isKnownType( type );
}

}  // namespace SceneEngine
