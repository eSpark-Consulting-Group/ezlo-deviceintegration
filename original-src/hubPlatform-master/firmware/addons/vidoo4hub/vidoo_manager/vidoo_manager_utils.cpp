#include "vidoo4hub/vidoo_manager/vidoo_manager_utils.h"
#include "cored/model/resources/cored_resources.h"

#include <algorithm>
#include <iterator>
#include <utility>
#include <map>
#include <memory>
#include <iomanip>

namespace HubPlatform {
namespace Vidoo4Hub {
namespace VidooManagerUtils {

ThreeWaySort::Result ThreeWaySort::sort( Keys keysA, Keys keysB ) {
    Result result;

    if( keysA.empty() ) {
        result.newKeys = std::move( keysB );
        return result;
    }

    if( keysB.empty() ) {
        result.missingKeys = std::move( keysA );
        return result;
    }

    auto itA = keysA.begin();
    auto endA = keysA.end();

    auto itB = keysB.begin();
    auto endB = keysB.end();

    while( itA != endA && itB != endB ) {
        if( *itA < *itB ) {
            ++itA;
        }
        else {
            if( !( *itB < *itA ) ) {
                result.commonKeys.emplace( *itA );
                keysA.erase( itA++ );
                keysB.erase( itB++ );
                continue;
            }
            ++itB;
        }
    }
    result.missingKeys = std::move( keysA );
    result.newKeys = std::move( keysB );

    return result;
}

std::string getTimeISO8601UTC() {
    std::time_t time = std::time( 0 );
    std::tm* tm = std::localtime( &time );
    if( tm ) {
        std::vector< char > buffer( 120, 0 );
        strftime( buffer.data(), buffer.size(), "%FT%TZ", tm );
        return std::string( buffer.data() );
    }
    return {};
}

bool updateJsonValue( Json& valueToUpdate, const Json& testValue, const std::string key ) {
    auto valueToUpdateIt = valueToUpdate.find( key );
    auto testValueIt = testValue.find( key );

    if( testValueIt == testValue.cend() )
        return false;

    if( valueToUpdateIt == valueToUpdate.end() ) {
        valueToUpdate[ testValueIt.key() ] = testValueIt.value();
        return true;
    }

    if( valueToUpdateIt.value() == testValueIt.value() )
        return false;

    *valueToUpdateIt = *testValueIt;
    return true;
}

std::set< std::string > getDictionaryKeys( const Json& dictionary ) {
    std::set< std::string > keys;
    for( auto it = dictionary.cbegin(); it != dictionary.cend(); ++it )
        keys.emplace( it.key() );
    return keys;
}

bool updateDetectableItemsDictionary( IDictionaryManager& dictionaryManager,
                                      const std::string& dictionaryId,
                                      Json oldDictionary,
                                      Json newDictionary ) {
    using namespace HubPlatform::CoreD::Model::Resources;

    auto sortedKeys = ThreeWaySort::sort( getDictionaryKeys( oldDictionary ), getDictionaryKeys( newDictionary ) );

    bool updateRequired = ( !sortedKeys.missingKeys.empty() ) || ( !sortedKeys.newKeys.empty() );
    Json dictionary;

    for( auto& key : sortedKeys.commonKeys ) {
        auto value = oldDictionary.at( key );
        updateRequired |= updateJsonValue( value, newDictionary.at( key ), DetectableObjectBaseFields::Name );
        updateRequired |= updateJsonValue( value, newDictionary.at( key ), DetectableObjectBaseFields::Value );
        dictionary[ key ] = std::move( value );
    }

    for( auto& key : sortedKeys.newKeys ) {
        dictionary[ key ] = newDictionary.at( key );
    }

    if( updateRequired ) {
        dictionaryManager.updateDictionary( dictionaryId, dictionary );
    }
    return updateRequired;
}

}  // namespace VidooManagerUtils
}  // namespace Vidoo4Hub
}  // namespace HubPlatform
