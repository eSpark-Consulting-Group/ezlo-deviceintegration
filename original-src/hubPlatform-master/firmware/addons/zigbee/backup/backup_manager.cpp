#include "zigbee/backup/backup_manager.h"
#include "zigbee/backup/backup_restore_status.h"

#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"
#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace Zigbee {
namespace Backup {

static constexpr uint16_t TCSO_INITIAL_SECURITY_BITMASK = 0x1B84;

BackupManager::BackupManager( ZigbeeAddonCallbackInterface& callbackInterface )
: m_callbackInterface( callbackInterface ) {}

void BackupManager::collectNetworkInfo() {
    ZigbeeNetworkInfo result;
    auto completed = collectCurrentNodeInformation( result );
    collectCurrentSecurityState( result );

    m_callbackInterface.networkInfoCollected( result, completed );
}

void BackupManager::restoreNetwork( const ZigbeeNetworkInfo& info ) {
    m_infoToRestore = std::make_unique< ZigbeeNetworkInfo >( info );
    emberLeaveNetwork();
}

bool BackupManager::isBackupRestoreInProgress() {
    return m_infoToRestore.get();
}

void BackupManager::onNetworkDown() {
    if( !m_infoToRestore ) {
        Log::error( cc::error( "Backup restore is not started" ) );
        return;
    }

    auto& info = *m_infoToRestore;

    // If a network already exists, TC replacement does not make sense.
    if( EMBER_NO_NETWORK != emberAfNetworkState() ) {
        Log::error( cc::error( "Cannot import TC data while network is up" ) );
        m_callbackInterface.restoreBackupFailed( BackupRestoreStatus::CannotImportTcData );
        return;
    }

    EmberInitialSecurityState state;
    EmberNetworkParameters params;
    EmberStatus emberStatus;
    EzspStatus ezspStatus;

    BackupRestoreStatus resultStatus = BackupRestoreStatus::Finished;

    // Copy security information into initialization format.
    Log::debug( cc::debug( "Setting security information" ) );
    MEMCOPY( emberKeyContents( &( state.networkKey ) ),
             emberKeyContents( &( info.networkKey ) ),
             EMBER_ENCRYPTION_KEY_SIZE );
    state.networkKeySequenceNumber = info.networkKeySeqNum;
    MEMCOPY( emberKeyContents( &( state.preconfiguredKey ) ),
             emberKeyContents( &( info.globalKey ) ),
             EMBER_ENCRYPTION_KEY_SIZE );

    // The initial security bitmask is a combination of existing configuration
    // parameters, hopefully based on the loaded data.
    // Because the parameters are manually loaded, there's no need to set the
    // tokens and the bitmask can be configured directly from the header file.
    state.bitmask = TCSO_INITIAL_SECURITY_BITMASK;

    // Set node information and network parameters.
    Log::debug( cc::debug( "Setting node information and network parameters" ) );
    MEMCOPY( params.extendedPanId, info.ePanId, sizeof( params.extendedPanId ) );
    params.panId = info.panId;
    params.radioTxPower = info.radioPower;
    params.radioChannel = info.radioChannel;
    params.joinMethod = EMBER_USE_MAC_ASSOCIATION;

    // Initialize the network security state.
    Log::debug( cc::debug( "Starting network security state" ) );
    emberStatus = emberSetInitialSecurityState( &state );
    if( emberStatus == EMBER_SUCCESS ) {
        Log::info( cc::info( "Successfully initialized network security state" ) );
    }
    else {
        Log::error( cc::error( "Failed to initialized network security state. Error: " ) + cc::num0x( emberStatus ) );
        resultStatus = BackupRestoreStatus::FailedToInitializedNetworkSecurityState;
    }

    // Reset the custom EUI64 to match that of the original trust center.
    // Note that the custom EUI64 cannot have been reset before, so if this fails
    // make sure that the manufacturing tokens have been cleared.
    emberStatus = ezspSetMfgToken( EZSP_MFG_CUSTOM_EUI_64, 8, info.eui64 );
    if( emberStatus == EMBER_SUCCESS ) {
        Log::info( cc::info( "Successfully set the EUI64" ) );
    }
    else {
        Log::warn( cc::warn( "Failed to set the EUI64, Error: " ) + cc::num0x( emberStatus ) );
    }

    // Set frame counter values.
    ezspStatus = ezspSetValue( EZSP_VALUE_NWK_FRAME_COUNTER, sizeof( int32u ), ( (int8u*) &info.networkFC ) );
    if( ezspStatus == EZSP_SUCCESS ) {
        Log::info( cc::info( "Successfully set the network frame counter" ) );
    }
    else {
        Log::error( cc::error( "Failed to set network frame counter. Error: " ) + cc::num0x( ezspStatus ) );
        resultStatus = BackupRestoreStatus::FailedToSetNetworkFrameCounter;
    }
    ezspStatus = ezspSetValue( EZSP_VALUE_APS_FRAME_COUNTER, sizeof( int32u ), ( (int8u*) &info.globalFC ) );
    if( ezspStatus == EZSP_SUCCESS ) {
        Log::info( cc::info( "Successfully set the global frame counter" ) );
    }
    else {
        Log::error( cc::error( "Failed to set global frame counter. Error: " ) + cc::num0x( ezspStatus ) );
        resultStatus = BackupRestoreStatus::FailedToSetGlobalFrameCounter;
    }

    // Now that the parameters have been configured, kick off the network.
    emberStatus = emberAfFormNetwork( &params );
    if( emberStatus == EMBER_SUCCESS ) {
        Log::info( cc::info( "Successfully formed network with given params" ) );
    }
    else {
        Log::error( cc::error( "Failed to form network. Error: " ) + cc::num0x( emberStatus ) );
        resultStatus = BackupRestoreStatus::FailedToFormNetwork;
    }

    m_infoToRestore.reset();
    if( resultStatus != BackupRestoreStatus::Finished ) {
        m_callbackInterface.restoreBackupFailed( resultStatus );
    }
    else {
        m_callbackInterface.restoreBackupCompleted();
    }
}

bool BackupManager::collectCurrentNodeInformation( ZigbeeNetworkInfo& info ) {
    EmberStatus getStatus;
    EmberNodeType nodeType;
    EmberNetworkParameters params;
    getStatus = emberAfGetNetworkParameters( &nodeType, &params );
    if( getStatus == EMBER_SUCCESS ) {
        info.panId = params.panId;
        info.nodeId = (unsigned int) EMBER_TRUST_CENTER_NODE_ID;
        info.radioChannel = params.radioChannel;
        info.radioPower = params.radioTxPower;
        MEMCOPY( &( info.ePanId ), &( params.extendedPanId ), sizeof( params.extendedPanId ) );
        EmberEUI64 value;
        emberLookupEui64ByNodeId( EMBER_TRUST_CENTER_NODE_ID, &value[ 0 ] );
        MEMCOPY( &( info.eui64 ), &( value ), sizeof( value ) );
        return true;
    }
    else {
        Log::error( cc::error( "Failed to retrieve node information to save" ) );
        return false;
    }
}

void BackupManager::collectCurrentSecurityState( ZigbeeNetworkInfo& info ) {
    EmberKeyStruct keyStruct;
    emberGetKey( EMBER_CURRENT_NETWORK_KEY, &keyStruct );
    info.networkKeySeqNum = keyStruct.sequenceNumber;
    info.networkFC = keyStruct.outgoingFrameCounter;
    info.networkKey = keyStruct.key;
    emberGetKey( EMBER_TRUST_CENTER_LINK_KEY, &keyStruct );
    info.globalKey = keyStruct.key;
    info.globalFC = keyStruct.outgoingFrameCounter;
}

}  // namespace Backup
}  // namespace Zigbee
}  // namespace HubPlatform
