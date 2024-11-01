#pragma once
#include "cored/db_modifier/idb_modifier.h"

#include "json_utils/json_utils.h"
#include "cored/what_happened.h"
#include "bus/messagesclasses.h"

#include <functional>

namespace HubPlatform {
namespace CoreD {

class DbModifier : public IDbModifier {
public:
    struct Dictionary {
        std::string id;
        Json value;
        std::string operation;

        bool empty() const {
            return operation.empty();
        }

        void clear() {
            id.clear();
            value.clear();
            operation.clear();
        }
    };

    using Broadcaster = std::function< void( MessageSubClasses, const Json& ) >;
    using Actions = std::vector< std::function< void( const Json& objDescription ) > >;

    struct OperationSpecifier {
        MessageSubClasses classType;
        Actions actions;
    };

    struct Broadcasts {
        OperationSpecifier addOperation;
        OperationSpecifier updateOperation;
        OperationSpecifier removeOperation;
    };

    DbModifier( IDbClient& db,
                Broadcaster broadcaster,
                std::string m_dbName,
                Broadcasts m_broadcasts,
                std::vector< std::string > m_dynamicFields );

    ~DbModifier() override = default;

    void insert( const Json& record ) override;
    void update( const Id& recordId, const Json& updatePortion, bool updateDynamicFields = true ) override;
    void remove( const Id& recordId ) override;

    /* @brief send broadcast if database is updated
     *  @param[in] subClass subclass of message
     *  @param[in] broadcastData
     */
    virtual void sendUpdateBroadcast( MessageSubClasses subClass,
                                      Json& broadcastData,
                                      const Json& /*oldFieldsValues*/ ) {
        m_broadcaster( subClass, broadcastData );
    }
    /* @brief prepares json for broadcast via modifying some fields and values
     * @return returns modified json */
    virtual Json& prepareInsertBroadcastMessage( Json& broadcastData );
    virtual Json& prepareRemoveBroadcastMessage( Json& broadcastData, const Json& removedRecord );
    virtual bool isUpdateBroadcastEmpty( const Json& broadcastData ) const;

    IDbClient& getDBClient() override;

    /* @brief checks whether there was something inserted or updated*/
    virtual bool hasChanges() const {
        return false;
    }

    /* @brief explicitly sets value of changed/unchanged status*/
    virtual void setChangedStatus( bool /*isChanged*/ ) {}
    virtual void addBroadcastTimestamp( Json& broadcastData );

protected:
    virtual bool isUsingProtectConfig();

private:
    std::string getProtectConfig( const std::string& itemId );

protected:
    IDbClient& m_db;
    std::string m_dbName;
    Broadcaster m_broadcaster;

private:
    Broadcasts m_broadcasts;
    const std::vector< std::string > m_dynamicFields;
};

}  // namespace CoreD
}  // namespace HubPlatform
