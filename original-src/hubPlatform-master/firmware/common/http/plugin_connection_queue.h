#pragma once

#include "tiny/include/yamc/yamc_shared_lock.hpp"
#include "tiny/include/yamc/alternate_shared_mutex.hpp"
#include "http/connection.h"

namespace HubPlatform {
namespace CommonHttp {

class SharedMutex : public yamc::alternate::basic_shared_mutex< yamc::rwlock::WriterPrefer > {};

using SharedLock = yamc::shared_lock< SharedMutex >;
using UniqueLock = std::unique_lock< SharedMutex >;

using ConnectionId = std::string;

class PluginConnectionQueue {
public:
    using PluginId = std::string;
    using ConnectionIdPair = std::pair< PluginId, ConnectionId >;
    using ConnectionIdPairList = std::list< ConnectionIdPair >;
    using ConnectionMap = std::map< ConnectionId, ConnectionPtr >;

public:
    bool askAddToMultiHandle( const PluginId& pluginId, const ConnectionId& connId );
    void pushBackToAdd( ConnectionIdPair idPair );
    void pushBackToRemove( ConnectionIdPair idPair );
    bool askRemoveFromMultiHandle( const PluginId& pluginId, const ConnectionId& connId );
    bool markForRemoval( ConnectionPtr connection );
    void extractCollections( ConnectionIdPairList& copyToAdd,
                             ConnectionIdPairList& copyToRemove,
                             ConnectionMap& copyMarkedForRemoval );

public:
    mutable SharedMutex m_queueMutex;
    ConnectionIdPairList m_queueToAdd;
    ConnectionIdPairList m_queueToRemove;
    ConnectionMap m_markedForRemoval;  //!< Stores shared pointers till they need to exist
};

}  // namespace CommonHttp
}  // namespace HubPlatform