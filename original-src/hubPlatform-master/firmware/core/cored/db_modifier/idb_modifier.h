#pragma once

#include "json_utils/json_wrapper.h"

class IDbClient;

namespace HubPlatform {
namespace CoreD {

class IDbModifier {
public:
    using Id = std::string;

    virtual ~IDbModifier() = default;

    /* @brief inserts a @c record into the db, sends a @c notifyObj json as an *_ADDED broadcast
     * @note set a @c notifyObj json as empty to send a @c record json as a broadcast */
    virtual void insert( const Json& record ) = 0;

    /* @brief updates a db's record with a @c recordId id by an @c updatePortion json,
     * sends an @c updatePortion json as an *_UPDATED broadcast
     */
    virtual void update( const Id& recordId, const Json& updatePortion, bool updateDynamicFields = true ) = 0;

    /* @brief remove a db's record with a @c recordId id,
     * sends an *_REMOVED broadcast */
    virtual void remove( const Id& recordId ) = 0;

    virtual IDbClient& getDBClient() = 0;
};

}  // namespace CoreD
}  // namespace HubPlatform
