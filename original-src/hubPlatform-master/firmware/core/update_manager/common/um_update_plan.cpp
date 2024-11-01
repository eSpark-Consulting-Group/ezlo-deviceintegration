#include "um_update_plan.h"

#include "json_utils/json_utils.h"
#include <set>

namespace HubPlatform {
namespace UpdateManager {

UpdatePlan UpdatePlan::fromJson( const JsonObject& json ) {
    try {
        jsonCheckArray( json, "actions" );
    }
    catch( const std::exception& exception ) {
        return {};
    }

    UpdatePlan plan;

    for( auto& step : json[ "actions" ] ) {
        auto operation = UpdateOperation::fromJson( step );
        if( !operation.isValid() )
            return {};

        if( !plan.steps.empty() ) {
            auto& previousOperation = plan.steps.back();
            if( previousOperation.source != operation.source )
                return {};

            if( previousOperation.snapshot != operation.snapshot )
                return {};
        }

        plan.steps.push_back( std::move( operation ) );
    }

    return plan;
}

JsonObject UpdatePlan::toJson() const {
    JsonArray stepsJson = JSON_EMPTY_ARR;

    for( const auto& step : steps ) {
        stepsJson.push_back( step.toJson() );
    }

    JsonObject json;
    json[ "actions" ] = stepsJson;
    return json;
}

bool UpdatePlan::isValid() const {
    return !steps.empty();
}

bool UpdatePlan::validateCustomPluginActions( const JsonObject& json,
                                              std::string& errorMessage,
                                              std::string& errorField ) {
    JsonArray actions;
    const char* const updatePlanFieldActions = "actions";
    if( json_get( json, updatePlanFieldActions, actions ) ) {
        if( !actions.is_array() ) {
            errorMessage = "field has incorrect format";
            errorField = updatePlanFieldActions;
            return false;
        }
    }
    else {
        errorMessage = "field doesn't exist";
        errorField = updatePlanFieldActions;
        return false;
    }

    auto isErrorField = [&errorMessage]( const JsonObject& json, const std::string& name ) -> bool {
        JsonObject obj;
        if( json_get( json, name, obj ) ) {
            if( !json[ name ].is_string() ) {
                errorMessage = "field has incorrect format";
            }
        }
        else {
            errorMessage = "field doesn't exist";
        }
        return !errorMessage.empty();
    };

    namespace Fields = Resources::UpdateOperationFields;
    std::set< std::string > customPluginKeysInstall = {
        Fields::Type, Fields::Id, Fields::Version, Fields::Source, Fields::ArchiveHashMd5
    };
    std::set< std::string > customPluginKeysUninstall = { Fields::Type, Fields::Id };

    for( const auto& action : actions ) {
        // Check "type" field first
        if( isErrorField( action, Fields::Type ) ) {
            errorField = Fields::Type;
            return false;
        }
        // Validate only actions for type = "custom_plugin". requirements for other types are different
        if( UpdateComponentType::CustomPlugin != updateComponentTypeFromString( action[ Fields::Type ] ) )
            continue;

        if( !isErrorField( action, Fields::Operation ) ) {
            const auto operation = json_get_str( action, Fields::Operation );
            auto operationType = updateOperationTypeFromString( operation );
            if( operationType == UpdateOperationType::Install || operationType == UpdateOperationType::Upgrade ) {
                for( const auto& key : customPluginKeysInstall ) {
                    if( isErrorField( action, key ) ) {
                        errorField = key;
                        return false;
                    }
                }
            }
            else if( operationType == UpdateOperationType::Uninstall ) {
                for( const auto& key : customPluginKeysUninstall ) {
                    if( isErrorField( action, key ) ) {
                        errorField = key;
                        return false;
                    }
                }
            }
            else if( operationType == UpdateOperationType::Unknown ) {
                errorMessage = "incorrect value: " + operation;
                errorField = Fields::Operation;
                return false;
            }
        }
        else {
            errorField = Fields::Operation;
            return false;
        }
    }

    return true;
}

}  // namespace UpdateManager
}  // namespace HubPlatform
