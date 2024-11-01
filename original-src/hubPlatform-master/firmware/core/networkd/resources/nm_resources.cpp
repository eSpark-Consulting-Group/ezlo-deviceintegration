#include "networkd/resources/nm_resources.h"
#include "networkd/resources/nm_public_resources.h"

#include "database_client/db_tables_names.h"

namespace HubPlatform {
namespace Network {
namespace Resources {

const std::array< std::pair< const char*, const char* >, 4 > BackupTableFieldPairs = { {
    { GSMManagerInterfacesDBName, "" },
    { SIMSettingsDBName, "pinCode" },
    { InterfaceSettingsDBName, "" },
    { WifiSettingsDBName, "key" },
} };

#ifdef __HUB_TYPE_G150__
const char CombinedInterfaceName[] = "apcli_wifi";
#endif

const char ApInterfaceName[] = "ap_wifi";
const char ClientInterfaceName[] = "client_wifi";
const char MobileBaseInterfaceName[] = "mobile";
const char MobileBaseDeviceName[] = "mobile";
const char ApIp[] = "172.27.64.20";
const char ApNetMask[] = "255.255.255.0";  // /24
const int ApStartLeasingAddress = 20 + 1;
const int ApLimitLeasingAddress = 150;

#ifdef __HUB_TYPE_G150__
const char EthernetInterfaceName[] = "wan";
const char EthernetDeviceName[] = "eth0.2";
const char ClientWifiDeviceName[] = "apcli0";
const char ApWifiDeviceName[] = "ra0";
const char DefaultApWifiChannel[] = "11";
#else
const char EthernetInterfaceName[] = "lan";
const char EthernetDeviceName[] = "eth0";
const char ClientWifiDeviceName[] = "wlan0";
const char ApWifiDeviceName[] = "wlan0-1";
const char DefaultApWifiChannel[] = "11";
#endif

const char DefaultWifiRegion[] = "00";
const char DefaultApEncryption[] = "psk2";

const std::vector< std::string >& DefaultDNSServers() {
    static const std::vector< std::string > servers = { "8.8.8.8", "1.1.1.1" };
    return servers;
}

const char WanNetwork[] = "wan";
const char LanNetwork[] = "lan";

const int EthernetPriority = 0;
const int WifiClientPriority = 1;
const int WifiApPriority = 2;
const int MobileBasePriority = 3;

const int EthernetMetric = 100;
const int WifiClientMetric = 200;
const int WifiApMetric = 300;
const int MobileBaseMetric = 400;

namespace Settings {
const char ModemSignalQualityPeriod[] = "broadcasts.modem_signal_quality.period";
const char* const PingTimeout = "network.ping_timeout";
}  // namespace Settings

namespace Scan {
const char ChannelKey[] = "channel";
const char BssidKey[] = "bssid";
const char SsidKey[] = "ssid";
const char SignalLevelValueKey[] = "signalLevel";
const char WifiSecurityKey[] = "security";
const char Aux[] = "aux";
namespace AuxFields {
const char Band[] = "band";
const char Frequency[] = "frequency";
const char Associated[] = "associated";
const char Signal[] = "signal";
const char Capabilities[] = "capabilities";
}  // namespace AuxFields
const int TryAttempts = 5;
}  // namespace Scan

namespace InterfaceInfo {

namespace Fields {
const char Status[] = "status";
const char Ipv4[] = "ipv4";
const char Ipv6[] = "ipv6";
const char Mask[] = "mask";
const char Ip[] = "ip";
const char Mode[] = "mode";
const char Gateway[] = "gateway";
const char Dns[] = "dns";
const char Addresses[] = "addresses";
const char PrefixLen[] = "prefixlen";
const char HwAddress[] = "hwaddr";
const char InternetAvailable[] = "internetAvailable";
const char Priority[] = "priority";
const char Metric[] = "metric";
const char Network[] = "network";
const char EnablePolicy[] = "enabled";
const char IMEI[] = "imei";
const char SIMInfo[] = "simInfo";
const char SignalQuality[] = "signalQuality";
const char AccessTechnology[] = "accessTechnology";
const char FrequencyBand[] = "frequencyBand";
const char DebugInformation[] = "~debugInformation";
}  // namespace Fields

namespace Statuses {
const char Unknown[] = "unknown";
const char Down[] = "down";
const char Up[] = "up";
}  // namespace Statuses

}  // namespace InterfaceInfo

namespace GSM {
const char GSMModemBaudRate[] = "115200";

const int EmbeddedCellularModemNumber = 0;

#ifdef __HUB_TYPE_H2_SECURE__
const char EmbeddedCellularModemPath[] = "1c1b000.usb/usb3/3-1";
#else
const char EmbeddedCellularModemPath[] = "";
#endif

#if( defined( __HUB_TYPE_H2__ ) || defined( __HUB_TYPE_H2_SECURE__ ) )
const int ExternalCellularModemsUSBPortPowerKeyPin = 2;
#endif

#if( defined( __HUB_TYPE_H2__ ) )
const int ExternalCellularModemsUSBBusNumber = 3;
#elif( defined( __HUB_TYPE_H2_SECURE__ ) )
const int ExternalCellularModemsUSBBusNumber = 5;
#elif( defined( __HUB_TYPE_G150__ ) )
const int ExternalCellularModemsUSBBusNumber = 1;
#endif

}  // namespace GSM

const int PingPeriod = 5;  // in seconds

}  // namespace Resources
}  // namespace Network
}  // namespace HubPlatform
