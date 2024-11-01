#include "response_builder_v1.h"

#include "cored/model/visitors/ui_value_writer_visitor.h"
#include "cored/model/visitors/ui_value_formatted_getter_visitior.h"
#include "cored/model/values/value.h"
#include "cored/model/resources/cored_resources.h"
#include "uid/response_builders/set_field_helper.h"

#include <cassert>

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {
namespace GetItems {
namespace V1 {

ResponseBuilder::ResponseBuilder( const rpc::Message& message,
                                  const std::string& api,
                                  CoreD::Model::Configuration& configuration,
                                  FilterDataListI* filter )
: ResponseBuilderBase( message, api )
, m_configuration( configuration )
, m_filter( filter ) {}

void ResponseBuilder::addItem( const std::string& id,
                               const std::string& deviceId,
                               const std::string& name,
                               bool hasGetter,
                               bool hasSetter,
                               bool show,
                               const CoreD::Model::Value& value,
                               const OptionalFields& optionalFields ) {
    using namespace CoreD::Model;

    JsonObject joItem = JSON_EMPTY_OBJ;
    joItem[ "_id" ] = id;

    SetFieldHelper helper( joItem, m_filter );
    helper.set( "deviceId", deviceId );
    helper.set( "name", name );

    helper.set( "hasGetter", hasGetter );
    helper.set( "hasSetter", hasSetter );
    helper.set( "show", show );

    size_t containerLevel = 0;
    for( auto type = &value.getType(); type && type->isContainer();
         type = type->getSubType().get(), ++containerLevel ) {
        const auto elementsMaxNumberIt = optionalFields.elementsMaxNumbers.find( containerLevel );
        if( elementsMaxNumberIt != optionalFields.elementsMaxNumbers.end() ) {
            if( containerLevel == 0 ) {
                helper.set( "elementsMaxNumber", elementsMaxNumberIt->second );
            }
            else if( containerLevel == 1 && type->isArray() ) {
                helper.set( "elementsMaxNumberPerArray", elementsMaxNumberIt->second );
            }
            else {
                assert( !"specific elements max numbers field is not implemented" );
                helper.set( "elementsMaxNumberPerArray", "elementsMaxNumber" + std::to_string( containerLevel + 1 ) );
            }
        }
    }

    if( !optionalFields.stringRestriction.empty() ) {
        helper.set( Resources::ItemFields::StringRestrictionFieldName, optionalFields.stringRestriction );
    }

    if( !optionalFields.userCodeRestriction.empty() ) {
        helper.set( Resources::ItemFields::UserCodeRestrictionFieldName, optionalFields.userCodeRestriction );
    }

    if( !optionalFields.userCodeModes.empty() ) {
        helper.set( Resources::ItemFields::UserCodeModesFieldName, optionalFields.userCodeModes );
    }

    if( !optionalFields.info.empty() ) {
        helper.set( Resources::ItemFields::InfoFieldName, optionalFields.info );
    }
    CoreD::Model::UiValueFormattedGetterVisitor visitor{ m_configuration };
    helper.set( "valueFormatted", visitor.getValue( value ) );

    CoreD::Model::UiValueWriterVisitor valueGetter{ joItem, m_configuration };
    if( helper.filter( "value" ) )
        valueGetter.writeValue( value, "value" );

    using namespace CoreD::Model::Resources;

    auto replaceOptionalField =
        [&]( const ValueUPtr& optionalValue, const std::string& itemField, const std::string& responseField ) -> void {
        assert( value.getType() == optionalValue->getType() );
        if( helper.filter( itemField ) ) {
            writeOptionalField( joItem, *optionalValue, itemField, responseField );
        }
    };

    if( optionalFields.valueMin ) {
        replaceOptionalField(
            optionalFields.valueMin, ItemFields::ValueMinFieldName, ItemFields::Response::MinValueFieldName );
    }
    if( optionalFields.valueMax ) {
        replaceOptionalField(
            optionalFields.valueMax, ItemFields::ValueMaxFieldName, ItemFields::Response::MaxValueFieldName );
    }

    if( !optionalFields.enumeration.empty() ) {
        helper.set( "enum", optionalFields.enumeration );
    }

    if( optionalFields.oneWeekDayCost != 0 ) {
        helper.set( "oneWeekDayCost", optionalFields.oneWeekDayCost );
    }

    if( optionalFields.oneShiftedWeekDayCost != 0 ) {
        helper.set( "oneShiftedWeekDayCost", optionalFields.oneShiftedWeekDayCost );
    }

    if( !optionalFields.valueIndent.empty() ) {
        helper.set( ItemFields::ValueIndentFieldName, optionalFields.valueIndent );
    }

    m_items.push_back( std::move( joItem ) );
}

void ResponseBuilder::writeOptionalField( JsonObject& response,
                                          CoreD::Model::Value& value,
                                          const std::string& itemField,
                                          const std::string& responseField ) {
    JsonObject joValue = JSON_EMPTY_OBJ;
    CoreD::Model::UiValueWriterVisitor valueGetter{ joValue, m_configuration };
    valueGetter.writeValue( value, itemField.c_str() );
    response[ responseField ] = joValue[ itemField ];
}

JsonObject ResponseBuilder::buildResponse() const {
    JsonObject joResponse = ResponseBuilderBase::buildResponse();
    joResponse[ "result" ][ "items" ] = m_items;
    return joResponse;
}

}  // namespace V1
}  // namespace GetItems

namespace DataListItems {
namespace V1 {

ResponseBuilder::ResponseBuilder( const rpc::Message& message,
                                  const std::string& api,
                                  CoreD::Model::Configuration& configuration,
                                  FilterDataListI* filter )
: GetItems::V1::ResponseBuilder( message, api, configuration, filter ) {}

void ResponseBuilder::addItem( const std::string& id,
                               const std::string& deviceId,
                               const std::string& name,
                               bool hasGetter,
                               bool hasSetter,
                               bool show,
                               const CoreD::Model::Value& value,
                               const OptionalFields& optionalFields ) {
    GetItems::V1::ResponseBuilder::addItem( id, deviceId, name, hasGetter, hasSetter, show, value, optionalFields );
}

JsonObject ResponseBuilder::buildResponse() const {
    JsonObject joResponse = ResponseBuilderBase::buildResponse();

    JsonObject result;

    for( auto item : m_items ) {
        std::string itemId = json_get_str( item, "_id" );
        if( !m_filter || m_filter->hasId( itemId ) ) {
            result[ itemId ] = makeFieldsDict( item, m_filter );
        }
    }

    fillResultMissedIds( result, m_filter, m_items, JsonObject( nullptr ) );

    joResponse[ "result" ][ "items" ] = result;
    return joResponse;
}

}  // namespace V1
}  // namespace DataListItems

}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform
