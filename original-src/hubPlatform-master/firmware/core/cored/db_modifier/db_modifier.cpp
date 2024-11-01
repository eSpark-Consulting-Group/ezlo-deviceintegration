#include "cored/db_modifier/db_modifier.h"
#include "cored/model/resources/cored_resources.h"

#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"

#include "base_utils/float_operations.h"
#include "db_modifier_utils.h"

#include <stdexcept>

namespace HubPlatform {
namespace CoreD {

namespace {
Json fillDifferentData( Json& broadcastData,
                        Json& changedFieldsOldValues,
                        const Json& originalData,
                        const Json& newValues ) {
    bool needToUpdate = true;
    Json differentFields;
    for( auto it = newValues.begin(); it != newValues.end(); ++it, needToUpdate = true ) {
        const auto itFind = originalData.find( it.key() );
        if( itFind != originalData.end() ) {
            if( itFind->is_number_float() && it->is_number_float() ) {
                float origValue = itFind->get< float >();
                float newValue = it->get< float >();
                if( areFloatsEqual( origValue, 0.0f ) ) {
                    needToUpdate = std::abs( newValue ) > 0.001f /* 0.1% */;
                }
                else {
                    float diff = std::abs( ( newValue - origValue ) / origValue );
                    needToUpdate = diff > 0.001f /* 0.1% */;
                }
            }
            else if( itFind.value() == it.value() ) {
                needToUpdate = false;
            }
        }

        if( needToUpdate ) {
            differentFields[ it.key() ] = it.value();
            broadcastData[ it.key() ] = it.value();
            if( itFind != originalData.end() ) {
                changedFieldsOldValues[ it.key() ] = itFind.value();
            }
        }
    }

    return differentFields;
}
}  // namespace

class DbModifierError : public std::runtime_error {
public:
    explicit DbModifierError( const std::string& errDescription )
    : runtime_error( "DbModifier: " + errDescription ) {}
};

DbModifier::DbModifier( IDbClient& db,
                        Broadcaster broadcaster,
                        std::string dbName,
                        Broadcasts broadcasts,
                        std::vector< std::string > dynamicFields )
: m_db( db )
, m_dbName( std::move( dbName ) )
, m_broadcaster( std::move( broadcaster ) )
, m_broadcasts( std::move( broadcasts ) )
, m_dynamicFields( std::move( dynamicFields ) ) {}

void DbModifier::insert( const Json& obj ) {
    auto dynamicFieldsObj = JSON_EMPTY_OBJ;
    Json res = JSON_EMPTY_OBJ;

    for( const auto& op : m_broadcasts.addOperation.actions ) {
        op( obj );
    }

    for( auto it = m_dynamicFields.begin(); it != m_dynamicFields.end(); ++it ) {
        if( json_get( obj, *it, res ) ) {
            dynamicFieldsObj[ *it ] = obj[ *it ];
        }
    }
    if( dynamicFieldsObj != JSON_EMPTY_OBJ ) {
        dynamicFieldsObj[ "_id" ] = obj[ "_id" ];
        m_db.insert( ( TemporaryTableNamePreffix + m_dbName ), dynamicFieldsObj );
    }
    m_db.insert( m_dbName, obj );

    JsonObject broadcastData = obj;
    broadcastData = prepareInsertBroadcastMessage( broadcastData );

    m_broadcaster( m_broadcasts.addOperation.classType, broadcastData );
}

void DbModifier::update( const Id& objId, const Json& updatePortion, bool updateDynamicFields ) {
    const auto query = JSON( db::ID_FIELD_NAME << objId );

    const auto record = m_db.findOne( m_dbName, query, nullptr );
    if( record.empty() ) {
        throw DbModifierError{ "fail to update a record in db '" + m_dbName + "', invalid id '" + objId +
                               "' of the record" };
    }

    auto nativeUpdateObj = updatePortion;
    auto dynamicFieldsUpdate = JSON_EMPTY_OBJ;
    if( updateDynamicFields ) {
        for( auto it = m_dynamicFields.begin(); it != m_dynamicFields.end(); ++it ) {
            auto itFind = updatePortion.find( *it );
            if( itFind != updatePortion.end() ) {
                dynamicFieldsUpdate[ itFind.key() ] = itFind.value();
                json_remove( nativeUpdateObj, *it );
            }
        }
    }

    JsonObject broadcastData;
    JsonObject oldValues;
    if( !nativeUpdateObj.empty() ) {
        const auto differentFields = fillDifferentData( broadcastData, oldValues, record, nativeUpdateObj );
        if( !differentFields.empty() ) {
            m_db.update( m_dbName, query, JSON( "$set" << differentFields ), false, false );
            setChangedStatus( true );
        }
    }

    if( !dynamicFieldsUpdate.empty() ) {
        const auto currentValues = m_db.findOne( ( TemporaryTableNamePreffix + m_dbName ), query, nullptr );
        if( currentValues.empty() ) {
            const auto differentFields = fillDifferentData( broadcastData, oldValues, record, dynamicFieldsUpdate );
            if( !differentFields.empty() ) {
                dynamicFieldsUpdate[ "_id" ] = objId;
                m_db.insert( ( TemporaryTableNamePreffix + m_dbName ), dynamicFieldsUpdate );
                setChangedStatus( true );
            }
        }
        else {
            const auto differentFields =
                fillDifferentData( broadcastData, oldValues, currentValues, dynamicFieldsUpdate );
            if( !differentFields.empty() ) {
                m_db.update(
                    ( TemporaryTableNamePreffix + m_dbName ), query, JSON( "$set" << differentFields ), false, false );
                setChangedStatus( true );
            }
        }
    }

    if( !isUpdateBroadcastEmpty( broadcastData ) ) {
        broadcastData[ db::ID_FIELD_NAME ] = objId;
        sendUpdateBroadcast( m_broadcasts.updateOperation.classType, broadcastData, oldValues );
    }
}

void DbModifier::remove( const Id& objId ) {
    const auto query = JSON( db::ID_FIELD_NAME << objId );
    const auto record = m_db.findOne( m_dbName, query, nullptr );
    if( record.empty() ) {
        throw DbModifierError{ "fail to update a record from db '" + m_dbName + "', invalid id '" + objId +
                               "' of the record" };
    }

    for( const auto& op : m_broadcasts.removeOperation.actions ) {
        op( record );
    }
    std::string protectConfig;
    if( isUsingProtectConfig() ) {
        protectConfig = DBModifierUtils::getProtectConfig( m_db, objId );
    }

    m_db.remove( m_dbName, query );
    m_db.remove( ( TemporaryTableNamePreffix + m_dbName ), query );
    JsonObject broadcastData = query;
    if( isUsingProtectConfig() ) {
        broadcastData[ Model::Resources::DeviceFields::ProtectConfig ] = protectConfig;
    }
    broadcastData = prepareRemoveBroadcastMessage( broadcastData, record );
    m_broadcaster( m_broadcasts.removeOperation.classType, broadcastData );
}

IDbClient& DbModifier::getDBClient() {
    return m_db;
}

Json& DbModifier::prepareInsertBroadcastMessage( Json& broadcastData ) {
    broadcastData[ Model::Resources::System::SyncNotification ] = true;
    return broadcastData;
}

Json& DbModifier::prepareRemoveBroadcastMessage( Json& broadcastData, const Json& removedRecord ) {
    broadcastData[ Model::Resources::System::SyncNotification ] = true;
    return broadcastData;
}

bool DbModifier::isUpdateBroadcastEmpty( const Json& broadcastData ) const {
    return broadcastData.empty();
}

void DbModifier::addBroadcastTimestamp( Json& broadcastData ) {
    using namespace std::chrono;
    int64_t now = duration_cast< milliseconds >( system_clock::now().time_since_epoch() ).count();
    broadcastData[ Model::Resources::System::FirmwareTimestampMs ] = now;
}

bool DbModifier::isUsingProtectConfig() {
    return false;
}

}  // namespace CoreD
}  // namespace HubPlatform
