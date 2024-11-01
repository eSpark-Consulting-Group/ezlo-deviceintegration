#pragma once

#include "json_utils/json_fwd.h"
#include "config_reader/config_reader.h"
#include "cored/remote_interface/cored_interface.h"
#include "cored/house_modes/house_mode_entry.h"
#include "cored/house_modes/protect_button.h"
#include "service_interfaces/service.h"

class IDbClient;

namespace HubPlatform {

namespace CoreD {
class IDbModifier;
}

namespace CoreDHouseModes {

constexpr const char HOUSE_MODE_ENTRY[] = "houseMode";
constexpr const char HOUSE_MODE_SWITCH_ENTRY[] = "switchTo";
constexpr const char HOUSE_MODE_TIME_TO_SWITCH[] = "timeIsLeftToSwitch";
constexpr const char HOUSE_MODE_SWITCH_DELAY[] = "switchToDelay";
constexpr const char HOUSE_MODE_ALARM_DELAY[] = "alarmDelay";
constexpr const char HOUSE_MODE_ENTRY_DELAY[] = "entryDelay";
constexpr const char HOUSE_MODE_ENTRY_DELAY_NORMAL[] = "normal";
constexpr const char HOUSE_MODE_ENTRY_DELAY_LONG_EXTENDED[] = "longExtended";
constexpr const char HOUSE_MODE_ENTRY_DELAY_EXTENDED[] = "extended";
constexpr const char HOUSE_MODE_ENTRY_DELAY_INSTANT[] = "instant";
constexpr const char HOUSE_MODE_ABORT_WINDOW[] = "abortWindow";
constexpr const char HOUSE_MODE_ABORT_WINDOW_DEFAULT[] = "default";
constexpr const char HOUSE_MODE_ABORT_WINDOW_MINIMUM[] = "minimum";
constexpr const char HOUSE_MODE_ABORT_WINDOW_MAXIMUM[] = "maximum";

using DeviceIDs = std::vector< std::string >;
using StatusDeviceByMode = std::map< std::string, bool >;

using DeviceItemID = std::pair< std::string, std::string >;
using DeviceItemIDs = std::vector< DeviceItemID >;
using namespace HubPlatform::CoreD;

void ensureHouseModesInDb( IDbClient& dbClient, const ConfigReader& configReader );

std::vector< HouseModeEntry > getHouseModesList( const IDbClient& dbClient );
std::unique_ptr< HouseModeEntry > getHouseMode( const IDbClient& dbClient,
                                                const std::string& modeId,
                                                bool absentModeIsError = false );
std::unique_ptr< HouseModeEntry > getHouseModeByName( const IDbClient& dbClient,
                                                      const std::string& modeName,
                                                      bool absentModeIsError = false );
std::unique_ptr< HouseModeEntry > getCurrentHouseMode( const IDbClient& dbClient );

DeviceIDs getArmedDeviceIDsAffectedByHouseModes( const IDbClient& dbClient );
DeviceIDs getAlarmDeviceIDsAffectedByHouseModes( const IDbClient& dbClient );
DeviceItemIDs getCamerasDeviceItemPairs( const IDbClient& dbClient, const Json& query = JSON_EMPTY_OBJ );

using ProcessDbResultsetItem = std::function< void( const Json& ) >;
size_t getCamerasDeviceItemPairs( const IDbClient& dbClient,
                                  const Json& query,
                                  const ProcessDbResultsetItem& callback );

DeviceIDs getDefaultDisarmedDeviceIDs( const IDbClient& dbClient, const std::string& modeId );
DeviceIDs getDefaultAlarmOffDeviceIDs( const IDbClient& dbClient, const std::string& modeId );
DeviceIDs getDefaultCamerasOffDeviceIDs( const IDbClient& dbClient, const std::string& modeId );

StatusDeviceByMode checkAlarmCategoryAffectedByHouseMode( const IDbClient& dbClient,
                                                          const std::string& category,
                                                          const std::string& subcategory );
StatusDeviceByMode checkCategoryAffectedByHouseMode( const IDbClient& dbClient,
                                                     const std::string& category,
                                                     const std::string& subcategory );

std::string getCurrentHouseModeId( const IDbClient& dbClient );
std::string getHouseModeSwitchTo( const IDbClient& dbClient );

void setSwitchDelay( IDbClient& dbClient, int delay );
void setAlarmDelay( IDbClient& dbClient, int delay );

/* @return 0 or bigger in success cases and less for errors */
int getSwitchDelay( IDbClient& dbClient );
int getSwitchDelayLeft( IDbClient& dbClient );
int getAlarmDelay( IDbClient& dbClient );
Json getEntryDelay( IDbClient& dbClient );
void setEntryDelay( Services::Service& broadcaster, IDbClient& dbClient, const JsonObject& params );
std::tuple< std::string, std::string > getAlarmPhase( IDbClient& dbClient );
std::string getSwitchToPhase( IDbClient& dbClient );
void setAlarmPhase( IDbClient& dbClient, const std::string& phase, const std::string& status );
void setSwitchToPhase( IDbClient& dbClient, const std::string& status );
void resetEntryDelay( Services::Service& broadcaster, IDbClient& dbClient );
JsonObject getDefaultEntryDelay();
int fetchEntryDelayValue( IDbClient& dbClient, const std::string& deviceId );
bool setDisarmedDefault( Services::Service& broadcaster,
                         IDbModifier& dBModifier,
                         const std::string& modeId,
                         bool disarmedDefault );
bool addDisarmedDevice( Services::Service& broadcaster,
                        IDbModifier& dBModifier,
                        const std::string& modeId,
                        const std::string& deviceId,
                        bool newDevice = false,
                        bool changeArmedState = true );
bool removeDisarmedDevice( Services::Service& broadcaster,
                           IDbModifier& dBModifier,
                           const std::string& modeId,
                           const std::string& deviceId,
                           bool changeArmedState = true );
bool setProtect( Services::Service& broadcaster, IDbModifier& dBModifier, const std::string& modeId, bool protect );
bool setProtectTimestamp( IDbModifier& dBModifier, const std::string& modeId, std::chrono::milliseconds timestamp );
bool protectButtonsSet( Services::Service& broadcaster, const std::string& deviceId, ProtectServiceEnum service );
std::vector< ProtectButton > getProtectButtons( IDbClient& dbClient );
std::pair< ProtectButton, bool > findProtectButtonById( IDbClient& dbClient, const std::string& deviceId );
bool removeProtectButton( IDbClient& dbClient, const std::string& deviceId );

bool protectDevicesAdd( Services::Service& broadcaster, const DeviceIDs& ids );
bool protectDevicesRemove( Services::Service& broadcaster, const DeviceIDs& ids );
DeviceIDs getProtectDevicesIds( IDbClient& dbClient, const Json& dbQuery );
bool removeProtectDevice( IDbClient& dbClient, const std::string& deviceId );
std::tuple< bool, std::vector< std::string >, JsonObject > readDeviceIdsParams( const JsonObject& params );

bool setNotifications( IDbClient& dbClient,
                       Services::Service& broadcaster,
                       IDbModifier& dBModifier,
                       const std::string& modeId,
                       bool notifyAll,
                       const std::vector< std::string >& userIds );
void sendHouseModeAlarmedBroadcast( Services::Service& broadcaster,
                                    const std::string& modeId,
                                    const std::string& deviceId );

void sendHouseModeAlarmedBroadcast( Services::Service& broadcaster,
                                    const std::string& modeId,
                                    const std::string& status,
                                    const std::string& phase,
                                    const std::string& deviceId,
                                    bool silent,
                                    bool chime,
                                    std::string& soundType,
                                    int pendingDelay = 0 );

bool addAlarmsOffDevice( IDbClient& dbClient, const std::string& modeId, const std::string& deviceId );
bool removeAlarmsOffDevice( IDbClient& dbClient, const std::string& modeId, const std::string& deviceId );

bool addCamerasOffDevice( IDbClient& dbClient, const std::string& modeId, const std::string& deviceId );
bool removeCamerasOffDevice( IDbClient& dbClient, const std::string& modeId, const std::string& deviceId );
bool addCamera( IDbClient& dbClient, const std::string& deviceId, const std::string& makeRecordingItemId );
bool removeCamera( IDbClient& dbClient, const std::string& deviceId );

bool addBypassDevices( Services::Service& broadcaster, const std::string& modeId, const DeviceIDs& deviceIds );
bool removeBypassDevices( Services::Service& broadcaster, const std::string& modeId, const DeviceIDs& deviceIds );
void removeAllBypassDevices( Services::Service& broadcaster, const std::string& modeId );

bool eraseDisarmedDevice( IDbClient& dbClient, const std::string& deviceId );

bool modifyDevice( Services::Service& broadcaster,
                   IDbModifier& dbModifier,
                   const std::string& deviceId,
                   const JsonObject& device );

bool addSecurityDevice( IDbClient& dbClient, const std::string& deviceId, const std::string& securityThreatItemId );
bool removeSecurityDevice( IDbClient& dbClient, const std::string& deviceId );
size_t getSecurityDeviceItemPairs( const IDbClient& dbClient,
                                   const Json& query,
                                   const ProcessDbResultsetItem& callback );

JsonArray findHouseModesArrayField( IDbClient& dbClient, const std::string& modeId, const char* fieldName );

JsonObject getHouseModesAlarmDevicesEvent( IDbClient& dbClient, const std::string& id );
JsonObject getHouseModesAlarmDevicesEvents( IDbClient& dbClient );
void deleteDeviceFromHouseModesAlarmedList( IDbClient& dbClient, const std::string& id );
void addDeviceToHouseModesAlarmedList( IDbClient& dbClient,
                                       const std::string& id,
                                       unsigned long timestamp,
                                       int delay,
                                       bool silentAlarm );
void sendsAlarmPhase( Services::Service& broadcaster, const std::string& phase );
void clearDeviceFromHouseModesAlarmedList( IDbClient& dbClient );
int64_t now();
void checkCommandTimestampDb( IDbClient& dbClient, const std::string& modeId, std::chrono::milliseconds timestamp );
bool checkCommandTimestampTempDb( IDbClient& dbClient, const std::string& modeId, std::chrono::milliseconds timestamp );
std::chrono::seconds getTimestamp();
Json getAbortWindow( IDbClient& dbClient );
}  // namespace CoreDHouseModes
}  // namespace HubPlatform
