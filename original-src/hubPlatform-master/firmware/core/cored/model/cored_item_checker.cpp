#include "cored/model/cored_item_checker.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value.h"
#include "cored/model/values/value_type.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/visitors/value_encrypter_visitor.h"
#include "cored/model/exceptions.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {
namespace ItemChecker {

std::string itemValueTest( const std::string& type, const Json& value ) {
    using namespace CoreD::Model;
    JsonObject answer = JSON_EMPTY_OBJ;

    if( !value.empty() ) {
        auto valueTypeEnum = ValueType::fromString( type );
        try {
            if( valueTypeEnum == ValueType::Enum::Dictionary ) {
                auto dictionaryString = ValueType::makeDictionaryType( ValueType::Enum::String );
                Model::ValueFactory::createItemValue( dictionaryString, value );
            }
            else {
                Model::ValueFactory::createItemValue( valueTypeEnum, value );
            }
        }
        catch( const CoreD::Model::UnexpectedJsonValueFormatError& ex ) {
            return std::string{ "Unexpected value format: " }.append( ex.what() );
        }
        catch( const CoreD::Model::UnexpectedValueType& ex ) {
            return std::string{ "Unexpected value type: " }.append( ex.what() );
        }
        catch( const CoreD::Model::InvalidValueScale& ex ) {
            return std::string{ "Invalid value scale: " }.append( ex.what() );
        }
        catch( const CoreD::Model::ScalesNotFoundError& ex ) {
            return std::string{ "Scale not found: " }.append( ex.what() );
        }
        catch( const CoreD::Model::ValueError& ex ) {
            return std::string{ "Value error: " }.append( ex.what() );
        }
        catch( const CoreD::Model::ItemDictionaryIdGenerationError& ex ) {
            return std::string{ "Item dictionary id generation error: " }.append( ex.what() );
        }
        catch( const CoreD::Model::CoreDModelError& ex ) {
            return std::string{ "Core model error: " }.append( ex.what() );
        }
        catch( const std::exception& ex ) {
            return std::string{ "Another value error: " }.append( ex.what() );
        }
        catch( ... ) {
            return std::string{ "Another value error" };
        }
    }
    return "";
}

}  // namespace ItemChecker
}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
