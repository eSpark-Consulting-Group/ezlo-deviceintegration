#pragma once

#include "vidoo4hub/dictionary_manager/dictionary_manager_interface.h"

#include <map>

namespace HubPlatform {
namespace UT {

class FakeDictionaryManager : public Vidoo4Hub::IDictionaryManager {
public:
    Json readDictionary( const std::string& dictionaryId ) override {
        auto it = m_dictionaryMap.find( dictionaryId );
        if( it != m_dictionaryMap.end() )
            return it->second;
        return {};
    }

    void clearDictionary( const std::string& dictionaryId ) override {
        auto it = m_dictionaryMap.find( dictionaryId );
        if( it != m_dictionaryMap.end() )
            m_dictionaryMap.erase( it );
    }

    void updateDictionary( const std::string& dictionaryId, const Json& dictionaryJson ) override {
        auto it = m_dictionaryMap.find( dictionaryId );
        if( it != m_dictionaryMap.end() ) {
            if( dictionaryJson.empty() ) {
                m_dictionaryMap.erase( it );
            }
            else {
                it->second = dictionaryJson;
            }
        }
        else {
            if( dictionaryJson.empty() ) {
                return;
            }
            m_dictionaryMap[ dictionaryId ] = dictionaryJson;
        }
    }

public:
    std::map< std::string, Json > m_dictionaryMap;
};

}  // namespace UT
}  // namespace HubPlatform
