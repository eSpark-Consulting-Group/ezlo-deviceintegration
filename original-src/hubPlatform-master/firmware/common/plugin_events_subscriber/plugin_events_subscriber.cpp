#include "plugin_events_subscriber/plugin_events_subscriber.h"

#include "common_utils/cpp14.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "database_client/dbid.h"

namespace HubPlatform {

namespace Fields {
const char Exclude[] = "exclude";
const char Rules[] = "rules";
const char Event[] = "event";
const char Accepts[] = "accepts";
const char Id[] = "id";
const char Remove[] = "remove";
}  // namespace Fields

Json PluginEventsSubscriber::FilterRule::toJson() const {
    auto js = JSON_EMPTY_OBJ;

    js[ Fields::Event ] = m_eventName;
    if( !m_accepts.empty() ) {
        js[ Fields::Accepts ] = m_accepts;
    }

    return js;
}

PluginEventsSubscriber::FilterRules PluginEventsSubscriber::createFilterRulesFromJson( const JsonArray& filterRules ) {
    if( !checkFilterRules( filterRules ) ) {
        throw std::runtime_error( "Invalid event filter rules" );
    }

    PluginEventsSubscriber::FilterRules eventsFilterRules;

    for( const auto& jsRule : filterRules ) {
        bool needRemove = false;
        const auto accepts = jsRule[ Fields::Accepts ];

        const auto eventName = json_get_str( jsRule, Fields::Event );
        if( eventName.empty() ) {
            needRemove = true;
        }

        auto id = json_get_str( jsRule, Fields::Id );
        if( id.empty() ) {
            id = db::DbId::genOID();
        }

        eventsFilterRules.emplace( std::make_pair( id, FilterRule( eventName, accepts, needRemove ) ) );
    }

    return eventsFilterRules;
}

bool PluginEventsSubscriber::checkFilterRules( const Json& filterRules ) {
    if( !filterRules.is_array() ) {
        Log::error( cc::error( "Event filter rules are not composed as an array" ) );
        return false;
    }

    if( filterRules.size() > 15 ) {
        Log::error( cc::error( "Too much event filter rules" ) );
        return false;
    }

    for( auto it = filterRules.begin(); it != filterRules.end(); it++ ) {
        const auto& elm = *it;

        if( !elm.is_object() ) {
            Log::error( cc::error( "Event filter rule is not an object/table" ) );
            return false;
        }

        if( elm.find( Fields::Event ) == elm.end() ) {
            Log::error( cc::error( "Event filter rule miss a " ) + cc::note( Fields::Event ) + cc::error( " field" ) );
            return false;
        }

        const auto idIt = elm.find( Fields::Id );
        if( idIt != elm.end() && !( *idIt ).is_string() ) {
            Log::error( cc::error( "Event filter rule has an invalid " ) + cc::note( Fields::Id ) +
                        cc::error( " field" ) );
            return false;
        }

        auto acceptsIt = elm.find( Fields::Accepts );
        if( acceptsIt != elm.end() ) {
            if( !( *acceptsIt ).is_array() || ( *acceptsIt ).empty() || ( *acceptsIt ).size() > 15 ) {
                Log::error( cc::error( "Event filter rule has an invalid " ) + cc::note( Fields::Accepts ) +
                            cc::error( " field" ) );
                return false;
            }

            for( auto acceptRuleIt = acceptsIt->begin(); acceptRuleIt != acceptsIt->end(); acceptRuleIt++ ) {
                if( !( *acceptRuleIt ).is_object() ) {
                    Log::error( cc::error( "Event filter rule has an invalid " ) + cc::note( Fields::Accepts ) +
                                cc::error( " field" ) );
                    return false;
                }
            }
        }
    }

    return true;
}

PluginEventsSubscriber::PluginEventsSubscriber( const std::string& interpreterName,
                                                const std::string& pluginId,
                                                const std::string& moduleName,
                                                const std::string& scriptName,
                                                const Json& eventsFilter )
: m_interpreterName( interpreterName )
, m_pluginId( pluginId )
, m_moduleName( moduleName )
, m_scriptName( scriptName ) {
    JsonArray filterRules;
    std::tie( filterRules, m_excludedRules, m_blockAllEvents ) = untieEventsFilterObject( eventsFilter );
    m_eventsFilterRules = createFilterRulesFromJson( filterRules );
}

PluginEventsSubscriber::PluginEventsSubscriber( const Json& json )
: PluginEventsSubscriber( json_get_str( json, "interpreterName" ),
                          json_get_str( json, "pluginId" ),
                          json_get_str( json, "moduleName" ),
                          json_get_str( json, "scriptName" ),
                          json.at( "eventsFilter" ) ) {}

Json PluginEventsSubscriber::toJson() const {
    return JSON( "interpreterName" << m_interpreterName << "pluginId" << m_pluginId << "scriptName" << m_scriptName
                                   << "eventsFilter" << getEventFilterJson() << "moduleName" << m_moduleName );
}

// generate the same eventsFilter Json, as was provided to a ctor, basing on the instance state
Json PluginEventsSubscriber::getEventFilterJson() const {
    if( m_eventsFilterRules.empty() ) {
        if( !m_excludedRules && !m_blockAllEvents ) {
            return JSON_EMPTY_OBJ;
        }

        return JSON( Fields::Exclude << m_excludedRules << Fields::Rules << JSON_EMPTY_OBJ );
    }

    return JSON( Fields::Exclude << m_excludedRules << Fields::Rules << getEventsFilterRulesAsJson() );
}

void PluginEventsSubscriber::notify( const Services::ScriptEvent& event, Notifier notifier ) const {
    if( !filterEvent( event ) ) {
        return;
    }

    auto data = prepareMethod( "executeScript" );
    data[ "params" ][ "script" ] = m_scriptName;
    data[ "params" ][ "scriptParams" ] = event.toJson();

    notifier( Services::Message{ m_interpreterName, JSON_RPC, CALL_METHOD, std::move( data ) } );
}

const PluginEventsSubscriber::PluginEventsSubsriberId& PluginEventsSubscriber::getId() const {
    return m_scriptName;
}

const PluginEventsSubscriber::PluginId& PluginEventsSubscriber::getPluginId() const {
    return m_pluginId;
}

const PluginEventsSubscriber::ModuleName& PluginEventsSubscriber::getModuleName() const {
    return m_moduleName;
}

PluginEventsSubscriber::FilterRules& PluginEventsSubscriber::getEventsFilterRules() {
    return m_eventsFilterRules;
}

JsonArray PluginEventsSubscriber::getEventsFilterRulesAsJson() const {
    JsonArray filterRules = JSON_EMPTY_ARR;

    for( const auto& rule : m_eventsFilterRules ) {
        auto js = rule.second.toJson();
        js[ Fields::Id ] = rule.first;
        filterRules.push_back( js );
    }

    return filterRules;
}

std::tuple< JsonArray, bool, bool > PluginEventsSubscriber::untieEventsFilterObject( const Json& eventsFilter ) {
    JsonArray eventFilterRules = JSON_EMPTY_ARR;
    bool excludedRules = false;
    bool blockAllEvents = false;

    if( !eventsFilter.is_object() ) {
        Log::error( cc::error( "Event filter obejct is not an object" ) );
        throw std::runtime_error( "Event filter obejct is not an object" );
    }

    if( eventsFilter.empty() ) {
        eventFilterRules = JSON_EMPTY_ARR;
        excludedRules = false;
    }
    else {
        auto RulesIt = eventsFilter.find( Fields::Rules );
        if( RulesIt == eventsFilter.end() ) {
            Log::error( cc::error( "Event filter object miss a " ) + cc::note( Fields::Rules ) +
                        cc::error( " field" ) );
            throw std::runtime_error( std::string( "Event filter obejct miss a " ) + Fields::Rules + " field" );
        }

        eventFilterRules = *RulesIt;
        excludedRules = json_get_bool( eventsFilter, Fields::Exclude, false );

        if( eventFilterRules.empty() ) {
            eventFilterRules = JSON_EMPTY_ARR;
            blockAllEvents = !excludedRules;
        }
    }

    return std::make_tuple( eventFilterRules, excludedRules, blockAllEvents );
}

bool PluginEventsSubscriber::filterEvent( const Services::ScriptEvent& event ) const {
    auto retValue = m_eventsFilterRules.empty() ? !m_blockAllEvents : m_excludedRules;

    for( const auto& rule : m_eventsFilterRules ) {
        if( eventMatch( event, rule.second ) ) {  // OR logic
            retValue = !retValue;
            break;
        }
    }

    if( !retValue ) {
        Log::trace( cc::trace( "Filter out/block an event " ) + cc::note( event.getName() ) + cc::trace( " for a " ) +
                    cc::note( getId() ) + cc::trace( " subscriber, due to event filter rules, mode: " ) +
                    cc::note( m_excludedRules ? "EXCLUDE" : "INCLUDE" ) );
    }

    return retValue;
}

bool PluginEventsSubscriber::eventMatch( const Services::ScriptEvent& event, const FilterRule& filterRule ) const {
    const auto jEvent = event.toJson();
    const auto eventName = event.getName();
    const auto& ruleEventName = filterRule.m_eventName;
    const auto& accepts = filterRule.m_accepts;

    if( eventName == ruleEventName ) {
        if( accepts.empty() ) {
            return true;
        }

        for( auto acceptIt = accepts.begin(); acceptIt != accepts.end(); acceptIt++ ) {
            bool match = true;

            for( auto ruleIt = acceptIt->begin(); ruleIt != acceptIt->end(); ruleIt++ ) {
                bool ruleValid = false;

                for( auto eventIt = jEvent.begin(); eventIt != jEvent.end(); eventIt++ ) {
                    if( ruleIt.key() == eventIt.key() && ruleIt.value() == eventIt.value() ) {
                        ruleValid = true;
                        break;
                    }

                    if( ruleIt.key() == eventIt.key() && ruleIt.value().is_object() ) {
                        ruleValid = true;
                        break;
                    }
                }

                if( !ruleValid ) {
                    match = false;
                    break;
                }
            }

            if( match ) {
                return true;
            }
        }
    }

    return false;
}

}  // namespace HubPlatform
