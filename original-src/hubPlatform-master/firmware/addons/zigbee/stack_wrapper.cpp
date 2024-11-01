extern "C" {
#include <Zigbee/definitions.h>
#include <app/builder/zigbeegw/znet-bookkeeping.h>
#include <protocol/zigbee/app/framework/include/af.h>
#include <protocol/zigbee/app/framework/include/af-types.h>
#include <protocol/zigbee/app/framework/plugin/network-creator/network-creator.h>
#include <protocol/zigbee/app/framework/plugin/network-creator-security/network-creator-security.h>
#include <protocol/zigbee/app/framework/util/attribute-storage.h>
#include <protocol/zigbee/app/framework/plugin/device-database/device-database.h>
#include <protocol/zigbee/app/framework/plugin/device-table/device-table.h>
#include <protocol/zigbee/app/framework/util/af-main.h>
#include <protocol/zigbee/app/framework/security/crypto-state.h>
#include <protocol/zigbee/app/util/serial/linux-serial.h>
#include <protocol/zigbee/app/util/ezsp/ezsp.h>
#include <protocol/zigbee/app/util/ezsp/ezsp-enum.h>
#include <protocol/zigbee/stack/include/error-def.h>
#include <platform/base/hal/micro/unix/compiler/gcc.h>
}

#include "zigbee/zigbee_log.h"
#include "zigbee/stack_wrapper.h"

#include "log_utils/ezlo_log.h"

namespace {
// void(*)() void function stub
static void doNothing() {}

}  // namespace

namespace ZigbeeStackWrapper {
static const EmberReleaseTypeStruct ReleaseTypes[] = { EMBER_RELEASE_TYPE_TO_STRING_STRUCT_DATA };

bool getStackVersion( int& returnCode ) {
    EmberVersion version;
    unsigned int i = 0;
    const char* typeText = NULL;
    returnCode = ezspGetVersionStruct( &version );
    if( returnCode != EZSP_SUCCESS )
        return false;

    while( ReleaseTypes[ i ].typeString != NULL ) {
        if( ReleaseTypes[ i ].typeNum == version.type ) {
            typeText = ReleaseTypes[ i ].typeString;
        }
        i++;
    }
    ZB_LOG_DEBUG( "Stack version: [ %d.%d.%d ( patch %d ) %s build %d ]\n",
                  version.major,
                  version.minor,
                  version.patch,
                  version.special,
                  ( typeText == nullptr ? "???" : typeText ),
                  version.build );

    return true;
}

void getNetworkParameters() {
    auto networkState = emberNetworkState();
    EmberNodeType nodeType;
    EmberNetworkParameters networkParameters;

    if( networkState == EMBER_NO_NETWORK ) {
        ZB_LOG_ERR( "emberNetworkState returned EMBER_NO_NETWORK\n" );
        emberAfPluginNetworkCreatorStart( true );
    }

    const auto status = ezspGetNetworkParameters( &nodeType, &networkParameters );
    if( status == EMBER_SUCCESS ) {
        unsigned char buffer[ 8 ];
        emberAfGetEui64( buffer );
        ZB_LOG_DEBUG( "getNetworkParameters: MAC = %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
                      buffer[ 7 ],
                      buffer[ 6 ],
                      buffer[ 5 ],
                      buffer[ 4 ],
                      buffer[ 3 ],
                      buffer[ 2 ],
                      buffer[ 1 ],
                      buffer[ 0 ] );
        memcpy( buffer, networkParameters.extendedPanId, 8 );
        ZB_LOG_DEBUG( "getNetworkParameters: ePanId = %02x%02x%02x%02x%02x%02x%02x%02x\n",
                      buffer[ 7 ],
                      buffer[ 6 ],
                      buffer[ 5 ],
                      buffer[ 4 ],
                      buffer[ 3 ],
                      buffer[ 2 ],
                      buffer[ 1 ],
                      buffer[ 0 ] );
        ZB_LOG_DEBUG( "getNetworkParameters: PanId = %04x\n", networkParameters.panId );
        ZB_LOG_DEBUG( "getNetworkParameters: nwkManagerId = %d\n", networkParameters.nwkManagerId );
        ZB_LOG_DEBUG( "getNetworkParameters: nwkUpdateId  = %d\n", networkParameters.nwkUpdateId );
        ZB_LOG_DEBUG( "getNetworkParameters: radioChannel = %d\n", networkParameters.radioChannel );

        auto keyGetter = []( EmberKeyType keyType ) {
            EmberKeyStruct xKey;
            std::string keyName;
            switch( keyType ) {
            case EMBER_TRUST_CENTER_LINK_KEY:
                keyName = "Trust center link key";
                break;
            case EMBER_CURRENT_NETWORK_KEY:
                keyName = "Network key (current)";
                break;
            case EMBER_NEXT_NETWORK_KEY:
                keyName = "Network key (next)";
                break;
            case EMBER_APPLICATION_LINK_KEY:
                keyName = "Application link key";
                break;
            default:
                keyName = "Unknown key";
                break;
            }

            if( EMBER_SUCCESS == emberGetKey( keyType, &xKey ) ) {
                ZB_LOG_DEBUG( "%s is", keyName.c_str() );
                for( int i = 0; i < sizeof( xKey.key.contents ); ++i ) {
                    ZB_LOG_DEBUG( ":%02X", xKey.key.contents[ i ] );
                }
                ZB_LOG_DEBUG( "\n" );
            }
            else {
                ZB_LOG_WARN( "Failed to get %s\n", keyName.c_str() );
            }
        };

        keyGetter( EMBER_TRUST_CENTER_LINK_KEY );
        keyGetter( EMBER_CURRENT_NETWORK_KEY );
        keyGetter( EMBER_NEXT_NETWORK_KEY );
        keyGetter( EMBER_APPLICATION_LINK_KEY );
    }
    else {
        ZB_LOG_ERR( "Failed to get network parameters: status = %d\n", status );
    }
}

extern "C" void emAfResetAndInitNCP();
extern "C" bool emberAfNcpNeedsReset();
extern "C" void emberAfNcpNeedsResetSet( bool value );

void resetAndInitNCP() {
    emAfResetAndInitNCP();
}

const int RESULT_SUCCESS = EMBER_SUCCESS;
int init( int argc, char** argv ) {
    int returnCode = RESULT_SUCCESS;

    ZB_LOG_DEBUG( "ZigBee init() started\n" );
    // Initialize the HAL
    halInit();
    INTERRUPTS_ON();  // Safe to enable interrupts at this point

    if( emberAfMainStartCallback( &returnCode, argc, argv ) ) {
        return returnCode;
    }

    returnCode = emberSerialInit( APP_SERIAL, BAUD_115200, PARITY_NONE, 1 );
    if( EMBER_SUCCESS != returnCode ) {
        ZB_LOG_ERR( "Failed emberSerialInit(): returnCode = %d\n", returnCode );
        return returnCode;
    }

    emberAfAppPrintln( "Reset info: %d (%p)", halGetResetInfo(), halGetResetString() );
    emberAfCoreFlush();

    // This will initialize the stack of networks maintained by the framework,
    // including setting the default network.
    emAfInitializeNetworkIndexStack();

    // We must initialize the endpoint information first so
    // that they are correctly added by emAfResetAndInitNCP();
    emberAfEndpointConfigure();

    // initialize the network co-processor (NCP)
    emAfResetAndInitNCP();

    // initialize the ZCL framework
    emAfInit();

    if( !getStackVersion( returnCode ) ) {
        ZB_LOG_WARN( "Failed getStackVersion(): returnCode = %d\n", returnCode );
        return returnCode;
    }

    emAfNetworkInit();

    getNetworkParameters();

    ZB_LOG_DEBUG( "ZigBee init() finished\n" );
    return returnCode;
}

void mainLoop() {
    // main loop
    halResetWatchdog();  // Periodically reset the watchdog.

    // see if the NCP has anything waiting to send us
    ezspTick();

    while( ezspCallbackPending() ) {
        ezspCallback();
    }

    // check if we have hit an EZSP Error and need to reset and init the NCP
    if( emberAfNcpNeedsReset() ) {
        // TODO we need this here: emberAfNcpNeedsResetSet( false );
        // re-initialize the NCP
        emAfResetAndInitNCP();
    }

    // Wait until ECC operations are done.  Don't allow any of the clusters
    // to send messages as the NCP is busy doing ECC
    if( emAfIsCryptoOperationInProgress() ) {
        return;  // continue;
    }

    // let the ZCL Utils run - this should go after ezspTick
    emAfTick();

    emberSerialBufferTick();

    emberAfRunEvents();

    // After each interation through the main loop, our network index stack
    // should be empty and we should be on the default network index again.
    emAfAssertNetworkIndexStackIsEmpty();
}

void permitJoining( unsigned char interval ) {
    if( interval > 0 ) {
        emberAfPluginNetworkCreatorSecurityOpenNetwork();
    }
    else {
        emberAfPluginNetworkCreatorSecurityCloseNetwork();
        emberAfPermitJoin( 0, true );
    }
}

void resetToDefaults() {
    ZB_LOG_DEBUG( "Stack wrapper: emberLeaveNetwork()\n" );

    emberClearBindingTable();
    emberAfClearReportTableCallback();
    for( uint8_t endpointIndex = 0; endpointIndex < emberAfEndpointCount(); endpointIndex++ ) {
        uint8_t endpoint = emberAfEndpointFromIndex( endpointIndex );
        emberAfResetAttributes( endpoint );
        emberAfGroupsClusterClearGroupTableCallback( endpoint );
        emberAfScenesClusterClearSceneTableCallback( endpoint );
    }

    for( uint16_t index = 0; index < EMBER_AF_PLUGIN_DEVICE_DATABASE_MAX_DEVICES; ++index ) {
        auto pDeviceInfo = emberAfPluginDeviceDatabaseGetDeviceByIndex( index );
        if( pDeviceInfo ) {
            emberAfPluginDeviceDatabaseEraseDevice( (uint8_t*) pDeviceInfo->eui64 );
        }
    }
    emberAfDeviceTableClear();
    emberLeaveNetwork();
}

void startNewNetwork() {
    ZB_LOG_DEBUG( "%s\n", __PRETTY_FUNCTION__ );
    auto networkState = emberNetworkState();
    ZB_LOG_ERR( "emberNetworkState: 0x%02x\n", networkState );
    if( networkState == EMBER_NO_NETWORK ) {
        emberAfPluginNetworkCreatorStart( true );
    }
}

}  // namespace ZigbeeStackWrapper
