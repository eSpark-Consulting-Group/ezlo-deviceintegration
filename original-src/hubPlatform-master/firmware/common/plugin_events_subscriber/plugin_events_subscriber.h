#pragma once

#include "service_interfaces/script_event.h"
#include "service_interfaces/message.h"

#include "json_utils/json_fwd.h"

#include <string>
#include <functional>
#include <unordered_map>
#include <tuple>

namespace HubPlatform {

class PluginEventsSubscriber final {
public:
    struct FilterRule {
        FilterRule() = default;
        FilterRule( const std::string& eventName, const Json& accepts, bool toRemove = false )
        : m_eventName( eventName )
        , m_accepts( accepts )
        , m_remove( toRemove ) {}

        Json toJson() const;

        std::string m_eventName;
        JsonArray m_accepts;
        bool m_remove = false;
    };

    using FilterRuleId = std::string;
    using FilterRules = std::unordered_map< FilterRuleId, FilterRule >;
    using Notifier = std::function< void( const Services::Message& ) >;
    using PluginEventsSubsriberId = std::string;
    using PluginId = std::string;
    using ModuleName = std::string;

    static FilterRules createFilterRulesFromJson( const JsonArray& filterRules );
    static bool checkFilterRules( const Json& filterRules );

    PluginEventsSubscriber() = default;
    PluginEventsSubscriber( const std::string& interpreterName,
                            const std::string& pluginId,
                            const std::string& moduleName,
                            const std::string& scriptName,
                            const Json& eventsFilter );

    explicit PluginEventsSubscriber( const Json& json );
    Json toJson() const;

    void notify( const Services::ScriptEvent& event, Notifier notifier ) const;

    const PluginEventsSubsriberId& getId() const;
    const PluginId& getPluginId() const;
    const ModuleName& getModuleName() const;
    FilterRules& getEventsFilterRules();
    JsonArray getEventsFilterRulesAsJson() const;

private:
    Json getEventFilterJson() const;
    std::tuple< JsonArray, bool, bool > untieEventsFilterObject( const Json& eventsFilter );
    bool filterEvent( const Services::ScriptEvent& event ) const;
    bool eventMatch( const Services::ScriptEvent& event, const FilterRule& filterRule ) const;

private:
    std::string m_interpreterName;
    std::string m_pluginId;
    std::string m_moduleName;
    std::string m_scriptName;
    bool m_excludedRules = false;  // by default PASS matching events and SKIP others
    bool m_blockAllEvents = false;
    FilterRules m_eventsFilterRules;
};

}  // namespace HubPlatform
