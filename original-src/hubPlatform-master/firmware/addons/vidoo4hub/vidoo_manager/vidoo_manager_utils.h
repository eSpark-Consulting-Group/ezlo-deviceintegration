#pragma once

#include "vidoo4hub/dictionary_manager/dictionary_manager_interface.h"
#include "cored/remote_interface/cored_interface.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Vidoo4Hub {
namespace VidooManagerUtils {
using CoreInterface = HubPlatform::CoreDRemoteInterface::CoreDInterface;

class ThreeWaySort {
public:
    using Keys = std::set< std::string >;
    struct Result {
        Keys missingKeys;
        Keys commonKeys;
        Keys newKeys;
    };

    ThreeWaySort() = delete;
    static Result sort( Keys keysA, Keys keysB );
};

std::string getTimeISO8601UTC();

bool updateJsonValue( Json& valueToUpdate, const Json& testValue, const std::string key );
std::set< std::string > getDictionaryKeys( const Json& dictionary );

bool updateDetectableItemsDictionary( IDictionaryManager& dictionaryManager,
                                      const std::string& dictionaryId,
                                      Json oldDictionary,
                                      Json newDictionary );

}  // namespace VidooManagerUtils
}  // namespace Vidoo4Hub
}  // namespace HubPlatform
