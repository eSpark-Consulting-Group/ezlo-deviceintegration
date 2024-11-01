#pragma once

#include "vidoo4hub/dictionary_manager/dictionary_manager_interface.h"

namespace HubPlatform {
namespace Vidoo4Hub {

class ICoreManager;

class DictionaryManager : public IDictionaryManager {
public:
    DictionaryManager( ICoreManager& coreManager );
    Json readDictionary( const std::string& dictionaryId ) override;
    void clearDictionary( const std::string& dictionaryId ) override;
    void updateDictionary( const std::string& dictionaryId, const Json& dictionaryJson ) override;

private:
    ICoreManager& m_coreManager;
};

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
