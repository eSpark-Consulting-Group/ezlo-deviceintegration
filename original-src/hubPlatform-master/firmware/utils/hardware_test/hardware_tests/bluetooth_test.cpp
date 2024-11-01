#include "hardware_test/hardware_tests/bluetooth_test.h"

#if defined( __HUB_TYPE_H2__ ) || defined( __HUB_TYPE_H2_SECURE__ )

#include "hardware_test/utils/test_log.h"

#include "base_utils/timeutils.h"

#include <gdbus.h>
#include <dbus/dbus.h>
#include <thread>
#include <future>

namespace HubPlatform {
namespace HardwareTest {

#define TEST_NAME "Bluetooth"
constexpr std::chrono::seconds TEST_TIMEOUT{ 80 };

namespace {
TestLog testLog( TEST_NAME );

constexpr const char AdapterInterface[] = "org.bluez.Adapter1";
constexpr const char DeviceInterface[] = "org.bluez.Device1";

constexpr const auto OperationsTimeout = std::chrono::seconds( 10 );
std::atomic_int foundDevices{ 0 };

void clientReady( GDBusClient* client, void* user_data ) {
    static_cast< std::promise< bool >* >( user_data )->set_value( true );
}

void deviceDetected( GDBusProxy* proxy, bool newDevice ) {
    DBusMessageIter iter;

    if( !g_dbus_proxy_get_property( proxy, "Address", &iter ) )
        return;

    const char* address;
    dbus_message_iter_get_basic( &iter, &address );

    if( !g_dbus_proxy_get_property( proxy, "RSSI", &iter ) )
        return;

    int16_t rssi;
    dbus_message_iter_get_basic( &iter, &rssi );

    std::string name = "(unknown)";
    if( g_dbus_proxy_get_property( proxy, "Name", &iter ) ) {
        const char* value;
        dbus_message_iter_get_basic( &iter, &value );
        name = value;
    }

    testLog() << ( newDevice ? "New   " : "Change" ) << " device  " << address << "  RSSI: " << rssi << "  " << name;
    foundDevices++;
}

void proxyAdded( GDBusProxy* proxy, void* user_data ) {
    const char* interface = g_dbus_proxy_get_interface( proxy );

    if( !strcmp( interface, AdapterInterface ) ) {
        *static_cast< GDBusProxy** >( user_data ) = proxy;
    }
    else if( !strcmp( interface, DeviceInterface ) ) {
        deviceDetected( proxy, true );
    }
}

void propertyChanged( GDBusProxy* proxy, const char* name, DBusMessageIter* iter, void* user_data ) {
    const char* interface = g_dbus_proxy_get_interface( proxy );
    if( !strcmp( interface, DeviceInterface ) ) {
        deviceDetected( proxy, false );
    }
}

void fillDiscoveryFilter( DBusMessageIter* iter, void* user_data ) {
    DBusMessageIter dict;

    dbus_message_iter_open_container( iter,
                                      DBUS_TYPE_ARRAY,
                                      DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING
                                          DBUS_TYPE_VARIANT_AS_STRING DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
                                      &dict );

    const char* transport = "le";
    g_dbus_dict_append_entry( &dict, "Transport", DBUS_TYPE_STRING, &transport );

    dbus_message_iter_close_container( iter, &dict );
}

void checkError( const DBusError* error, void* user_data ) {
    auto& errorMessage = *static_cast< std::promise< std::string >* >( user_data );

    if( dbus_error_is_set( error ) ) {
        errorMessage.set_value( error->name );
        return;
    }

    errorMessage.set_value( {} );
}

void checkReply( DBusMessage* message, void* user_data ) {
    auto& errorMessage = *static_cast< std::promise< std::string >* >( user_data );
    DBusError error;
    dbus_error_init( &error );
    if( dbus_set_error_from_message( &error, message ) == TRUE ) {
        errorMessage.set_value( error.name );
        dbus_error_free( &error );
        return;
    }
    errorMessage.set_value( {} );
}

void checkPromise( std::promise< std::string >& errorMessage, const std::string& prefix ) {
    auto errorMessageFuture = errorMessage.get_future();
    if( errorMessageFuture.wait_for( OperationsTimeout ) != std::future_status::ready ) {
        throw std::runtime_error( prefix + ": timeout" );
    }
    auto message = errorMessageFuture.get();
    if( !message.empty() ) {
        throw std::runtime_error( prefix + ": " + message );
    }
}

void setDiscoveryFilter( GDBusProxy* proxy ) {
    std::promise< std::string > errorMessage;
    std::string errorPrefix = "Failed to set discovery filter";

    if( !g_dbus_proxy_method_call(
            proxy, "SetDiscoveryFilter", fillDiscoveryFilter, checkReply, &errorMessage, nullptr ) ) {
        throw std::runtime_error( errorPrefix );
    }
    checkPromise( errorMessage, errorPrefix );
}

void powerOn( GDBusProxy* proxy ) {
    DBusMessageIter iter;

    if( !g_dbus_proxy_get_property( proxy, "Powered", &iter ) ) {
        return;
    }
    dbus_bool_t powered;
    dbus_message_iter_get_basic( &iter, &powered );
    if( powered ) {
        return;
    }
    std::promise< std::string > errorMessage;
    std::string errorPrefix = "Failed to power on adapter";
    powered = true;
    testLog() << "Powering on adapter...";
    if( !g_dbus_proxy_set_property_basic(
            proxy, "Powered", DBUS_TYPE_BOOLEAN, &powered, checkError, &errorMessage, nullptr ) ) {
        throw std::runtime_error( errorPrefix );
    }
    checkPromise( errorMessage, errorPrefix );
}

void scan( GDBusProxy* proxy, bool enable ) {
    std::promise< std::string > errorMessage;
    const char* method = enable ? "StartDiscovery" : "StopDiscovery";
    std::string errorPrefix = "Failed to " + std::string( enable ? "start" : "stop" ) + " discovery";

    if( !g_dbus_proxy_method_call( proxy, method, nullptr, checkReply, &errorMessage, nullptr ) ) {
        throw std::runtime_error( errorPrefix );
    }
    checkPromise( errorMessage, errorPrefix );
}

}  // namespace

BluetoothTest::BluetoothTest( const volatile sig_atomic_t& exitSignalRef )
: HardwareTestBase( TEST_NAME, TEST_TIMEOUT, exitSignalRef ) {}

HardwareTestInterface::TestResult BluetoothTest::run() {
    auto make_test = []( const char* name ) { return JSON( "name" << name << "result" << false ); };
    auto pass = []( JsonObject& j ) { j[ "result" ] = true; };
    TestResult result = HardwareTestInterface::TestResult::UNKNOWN;

    auto testGetMac = make_test( "Bluetooth get mac" );
    auto testBLEScan = make_test( "Bluetooth LE Scan" );
    auto testResults = make_test( m_name.c_str() );

    testLog() << "Starting " << m_name << " test...";

    auto startTestTime = TimeUtils::getMonotonicTime();
    {
        std::thread loopThread;
        std::unique_ptr< DBusConnection, decltype( &dbus_connection_unref ) > connection( nullptr,
                                                                                          dbus_connection_unref );
        std::unique_ptr< GDBusClient, decltype( &g_dbus_client_unref ) > client( nullptr, g_dbus_client_unref );
        std::unique_ptr< GMainLoop, decltype( &g_main_loop_unref ) > mainLoop( nullptr, g_main_loop_unref );
        try {
            foundDevices = 0;

            GDBusProxy* proxy = nullptr;
            std::promise< bool > ready;

            connection.reset( g_dbus_setup_bus( DBUS_BUS_SYSTEM, nullptr, nullptr ) );
            g_dbus_attach_object_manager( connection.get() );

            client.reset( g_dbus_client_new( connection.get(), "org.bluez", "/org/bluez" ) );

            g_dbus_client_set_ready_watch( client.get(), clientReady, &ready );
            g_dbus_client_set_proxy_handlers( client.get(), proxyAdded, nullptr, propertyChanged, &proxy );

            loopThread = std::thread( [&]() {
                mainLoop.reset( g_main_loop_new( nullptr, FALSE ) );
                g_main_loop_run( mainLoop.get() );
            } );

            auto readyFuture = ready.get_future();
            if( readyFuture.wait_for( OperationsTimeout ) != std::future_status::ready ) {
                throw std::runtime_error( "Bluetooth initialization failed" );
            }

            if( !proxy ) {
                throw std::runtime_error( "No bluetooth adapters found" );
            }

            DBusMessageIter iter;
            if( !g_dbus_proxy_get_property( proxy, "Address", &iter ) ) {
                throw std::runtime_error( "Can not get adapter mac address" );
            }

            const char* address;
            dbus_message_iter_get_basic( &iter, &address );
            testLog() << "Bluetooth MAC address: " << address;
            testGetMac[ "mac" ] = address;
            pass( testGetMac );

            powerOn( proxy );

            setDiscoveryFilter( proxy );
            testLog() << "Starting scan...";
            scan( proxy, true );

            for( int i = 0; i < 50; ++i ) {
                std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
                if( foundDevices >= 8 ) {
                    testLog() << "Found 8+ new/changed devices, stopping...";
                    break;
                }
            }

            if( foundDevices == 0 ) {
                throw std::runtime_error( "No BLE devices found" );
            }

            scan( proxy, false );

            pass( testBLEScan );
            result = TestResult::DONE;
        }
        catch( std::runtime_error& e ) {
            result = TestResult::FAILED;
            testLog() << e.what();
        }

        g_main_loop_quit( mainLoop.get() );
        if( loopThread.joinable() ) {
            loopThread.join();
        }
    }

    auto testTime = TimeUtils::getDiffMonotonicTime( startTestTime );
    testLog() << getTestResultReport( result, testTime );

    testResults[ "result" ] = ( result == TestResult::DONE ) ? true : false;
    JsonArray subtests = { testGetMac, testBLEScan };
    size_t passed = std::count_if( subtests.begin(), subtests.end(), []( JsonObject& j ) { return j[ "result" ]; } );
    testResults[ "value" ] = passed;
    testResults[ "threshold" ] = subtests.size();
    testResults[ "subtests" ] = std::move( subtests );

    testLog() << getTestResultJsonReport( testResults );

    return result;
}

}  // namespace HardwareTest
}  // namespace HubPlatform

#else

namespace HubPlatform {
namespace HardwareTest {

BluetoothTest::BluetoothTest( const volatile sig_atomic_t& exitSignalRef )
: HardwareTestBase( "DummyBluetooth", std::chrono::seconds( 1 ), exitSignalRef ) {}

HardwareTestInterface::TestResult BluetoothTest::run() {
    return HardwareTestInterface::TestResult::DONE;
}

}  // namespace HardwareTest
}  // namespace HubPlatform

#endif
