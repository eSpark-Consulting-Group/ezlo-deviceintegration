#pragma once

#include "addon_interfaces/addon.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {

namespace UT {
class BluetoothAddonTestSuite;
}

class IBluetoothManager;
class IBluetoothEnvironment;

class BluetoothAddon final : public Addons::Addon {
    friend class UT::BluetoothAddonTestSuite;

public:
    template < typename T >
    struct BluetoothManagerTypeTag {};

    template < typename T >
    struct BluetoothEnvironmentTypeTag {};

public:
    BluetoothAddon();
    ~BluetoothAddon() final;

    CommonUtils::AddonVersion getVersion() const override;

protected:
    void setUp() override;
    void handleMethodCall( const JsonObject& request,
                           MessageSubClasses requestType,
                           const std::string& sender ) override;

private:
    template < typename BluetoothManagerT, typename BluetoothEnvironmentT >
    BluetoothAddon( const std::string& name,
                    std::unique_ptr< Services::BusWrapper > busWrapper,
                    std::unique_ptr< IDbClient > dbClient,
                    BluetoothManagerTypeTag< BluetoothManagerT >,
                    BluetoothEnvironmentTypeTag< BluetoothEnvironmentT > );

    void handleLegacyMethodCall( const JsonObject& json, const std::string& /*sender*/ ) override;

    IBluetoothManager& getManager();

public:
    static const char* const Name;

    static const char* const StartDiscoveryMethod;
    static const char* const StopDiscoveryMethod;
    static const char* const RemoveDeviceMethod;
    static const char* const ConnectToDeviceMethod;
    static const char* const DisconnectDeviceMethod;
    static const char* const WriteCharacteristicMethod;
    static const char* const ReadCharacteristicMethod;
    static const char* const PairMethod;
    static const char* const StartNotifyMethod;
    static const char* const StopNotifyMethod;
    static const char* const MaintainConnectionMethod;

private:
    std::unique_ptr< IBluetoothManager > m_bluetoothManager;
    std::unique_ptr< IBluetoothEnvironment > m_bluetoothEnvironment;
};

template < typename BluetoothManagerT, typename BluetoothEnvironmentT >
inline BluetoothAddon::BluetoothAddon( const std::string& name,
                                       std::unique_ptr< Services::BusWrapper > busWrapper,
                                       std::unique_ptr< IDbClient > dbClient,
                                       BluetoothManagerTypeTag< BluetoothManagerT >,
                                       BluetoothEnvironmentTypeTag< BluetoothEnvironmentT > )
: Addon( name, std::move( busWrapper ), std::move( dbClient ) ) {
    m_bluetoothEnvironment = std::make_unique< BluetoothEnvironmentT >( *this );
    m_bluetoothManager = std::make_unique< BluetoothManagerT >( *m_bluetoothEnvironment );
}

}  // namespace HubPlatform
