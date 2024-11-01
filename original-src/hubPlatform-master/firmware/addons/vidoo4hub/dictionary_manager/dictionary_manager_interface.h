#pragma once

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Vidoo4Hub {

class IDictionaryManager {
public:
    virtual ~IDictionaryManager() = default;
    virtual Json readDictionary( const std::string& dictionaryId ) = 0;
    virtual void clearDictionary( const std::string& dictionaryId ) = 0;
    virtual void updateDictionary( const std::string& dictionaryId, const Json& dictionaryJson ) = 0;
};

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
