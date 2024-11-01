#pragma once

// hub types are defined relying on documents linked below
// -
// https://ezloproject.atlassian.net/wiki/spaces/PM/pages/69305412/eZLO+hub+PCBs+-+vision#eZLOhubPCBs-vision-Hubversions
// - https://ezloproject.atlassian.net/browse/EB-432

constexpr char BoxA20HubTypeVersion1[] = "a20.1";
constexpr char BoxH2PlusHubTypeVersion1[] = "h2.1";
constexpr char BoxH2SecureHubTypeVersion1[] = "h2_secure.1";
constexpr char BoxH3HubTypeVersion1[] = "h3.1";
constexpr char BoxLocalHubTypeVersion1[] = "local";
constexpr char BoxVHubTypeVersion1[] = "vhub";

constexpr char BoxG150HubTypeVersion1[] = "g150";
constexpr char BoxG450HubTypeVersion1[] = "g450";
constexpr char BoxG550HubTypeVersion1[] = "g550";

constexpr char BoxBeeWaveHubTypeVersion1[] = "beewave.1";

#if defined( __HUB_TYPE_A20__ )
constexpr const char* BoxHubTypeBuildVersion = BoxA20HubTypeVersion1;
#elif defined( __HUB_TYPE_H2__ )
constexpr const char* BoxHubTypeBuildVersion = BoxH2PlusHubTypeVersion1;
#elif defined( __HUB_TYPE_H2_SECURE__ )
constexpr const char* BoxHubTypeBuildVersion = BoxH2SecureHubTypeVersion1;
#elif defined( __HUB_TYPE_G150__ )
constexpr const char* BoxHubTypeBuildVersion = BoxG150HubTypeVersion1;
#elif defined( __HUB_TYPE_G450__ )
constexpr const char* BoxHubTypeBuildVersion = BoxG450HubTypeVersion1;
#elif defined( __HUB_TYPE_G550__ )
constexpr const char* BoxHubTypeBuildVersion = BoxG550HubTypeVersion1;
#elif defined( __HUB_TYPE_BEEWAVE__ )
constexpr const char* BoxHubTypeBuildVersion = BoxBeeWaveHubTypeVersion1;
#elif defined( __HUB_TYPE_LOCAL__ )
constexpr const char* BoxHubTypeBuildVersion = BoxLocalHubTypeVersion1;
#elif defined( __HUB_TYPE_VHUB__ )
constexpr const char* BoxHubTypeBuildVersion = BoxVHubTypeVersion1;
#endif

// following hub types are not applicable for linux code
// constexpr char AtomESP8266HubTypeVersion1[] = "8266.1";
// constexpr char PlughubHubTypeVersion1[]     = "8266.2";

// constexpr char AtomESP32HubTypeVersion1[]   = "32.1";
