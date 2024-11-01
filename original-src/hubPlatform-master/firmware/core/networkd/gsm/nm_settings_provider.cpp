#include "networkd/gsm/nm_settings_provider.h"

#include "log_utils/ezlo_log.h"
#include "tiny/include/tiny/tiny_consolecolors.h"
#include "json_utils/json_utils.h"
#include "database_client/dbqueryparser.h"

#include <algorithm>
#include <utility>

namespace HubPlatform {
namespace Network {
namespace GSM {

SettingsProvider::Setting::Setting( std::string apnName,
                                    std::string dialNumber,
                                    std::string pinCode,
                                    std::string operatorName,
                                    std::string pdpType )
: m_apnName( std::move( apnName ) )
, m_dialNumber( std::move( dialNumber ) )
, m_pinCode( std::move( pinCode ) )
, m_operatorName( std::move( operatorName ) )
, m_pdpType( std::move( pdpType ) ) {}

SettingsProvider::Setting::Setting( const Json& json ) {
    m_apnName = json_get_str( json, "apnName" );
    m_dialNumber = json_get_str( json, "dialNumber" );
    m_pinCode = json_get_str( json, "pinCode" );
    m_operatorName = json_get_str( json, "operator" );
    m_pdpType = json_get_str( json, "pdpType" );
}

SettingsProvider::Setting::operator bool() const {
    return !m_apnName.empty() || !m_dialNumber.empty() || !m_pinCode.empty() || !m_operatorName.empty() ||
           !m_pdpType.empty();
}

bool SettingsProvider::Setting::operator!() const {
    return !this->operator bool();
}

Json SettingsProvider::Setting::toJson() const {
    return JSON( "apnName" << m_apnName << "dialNumber" << m_dialNumber << "operator" << m_operatorName << "pdpType"
                           << m_pdpType );
}

SettingsProvider::SettingsProvider( std::string interfaceName,
                                    IDbClient& dbClient,
                                    GSMModem& gsmModem,
                                    const ProvisionDataUtils::ProvisionData& provisionData,
                                    const NetworkConfigFileManager::MobileConfiguration& configuration )
: m_interfaceName( std::move( interfaceName ) )
, m_configuration( configuration )
, m_dbClient( dbClient )
, m_gsmModem( gsmModem )
, m_provisionData( provisionData ) {}

void SettingsProvider::init() {
    m_simICCID = m_gsmModem.getSIMInfo( true ).m_iccid;
}

void SettingsProvider::reconstructSettings() {
    constructProposalSettings();
    selectFirstSettingsProfile();
}

SettingsProvider::Setting SettingsProvider::getCurrentSetting() const {
    return m_settings.size() > m_currentSetting ? m_settings[ m_currentSetting ] : Setting();
}

SettingsProvider::Setting SettingsProvider::getNextSetting() {
    auto setting = getCurrentSetting();
    m_currentSetting++;

    return setting;
}

void SettingsProvider::selectFirstSettingsProfile() {
    m_currentSetting = 0;
}

void SettingsProvider::setDatabaseUserSetting( const Setting& setting ) {
    if( !setting ) {
        LOGGER_DEBUG_RAW( cc::debug( "Skip setting empty user settings: " ) + cc::note( setting.toJson().dump( 1 ) ) +
                          cc::debug( ", interface: " ) + cc::note( m_interfaceName ) );
        return;
    }

    const auto query = JSON( db::ID_FIELD_NAME << m_simICCID );

    auto settings = m_dbClient.findOne( SIMSettingsDBName, query, nullptr );
    if( settings.empty() ) {
        settings[ db::ID_FIELD_NAME ] = m_simICCID;

        // keep up to m_simSettingsStorageSize SIM settings in the database
        if( m_dbClient.size( SIMSettingsDBName ) >= m_simSettingsStorageSize ) {
            const auto entries = m_dbClient.findAll( SIMSettingsDBName, JSON_EMPTY_OBJ, nullptr );
            m_dbClient.remove( SIMSettingsDBName, entries[ 0 ] );
        }
    }

    if( !setting.m_pinCode.empty() ) {
        settings[ "pinCode" ] = m_provisionData.encryptSensitiveData( setting.m_pinCode );
    }
    if( !setting.m_apnName.empty() ) {
        settings[ "apnName" ] = setting.m_apnName;
    }
    if( !setting.m_dialNumber.empty() ) {
        settings[ "dialNumber" ] = setting.m_dialNumber;
    }
    if( !setting.m_operatorName.empty() ) {
        settings[ "operator" ] = setting.m_operatorName;
    }
    if( !setting.m_pdpType.empty() ) {
        settings[ "pdpType" ] = setting.m_pdpType;
    }

    m_dbClient.update( SIMSettingsDBName, JSON_EMPTY_OBJ, settings, true );
}

SettingsProvider::Setting SettingsProvider::getDatabaseUserSetting() const {
    const auto query = JSON( db::ID_FIELD_NAME << m_simICCID );

    auto setting = m_dbClient.findOne( SIMSettingsDBName, query, nullptr );
    if( setting.empty() ) {
        return Setting();
    }

    if( setting.count( "pinCode" ) ) {
        setting[ "pinCode" ] = m_provisionData.decryptSensitiveData( json_get_str( setting, "pinCode" ) );
    }

    return Setting( setting );
}

void SettingsProvider::resetDatabaseUserSetting() {
    const auto query = JSON( db::ID_FIELD_NAME << m_simICCID );
    m_dbClient.remove( SIMSettingsDBName, query );
}

SettingsProvider::Setting SettingsProvider::combineSettings( const Setting& baseSetting,
                                                             const Setting& auxSetting ) const {
    static auto combine = []( std::string& s1, const std::string& s2 ) {
        if( s1.empty() )
            s1 = s2;
    };

    if( !auxSetting ) {
        return baseSetting;
    }

    Setting resultSetting = baseSetting;
    combine( resultSetting.m_operatorName, auxSetting.m_operatorName );
    combine( resultSetting.m_apnName, auxSetting.m_apnName );
    combine( resultSetting.m_dialNumber, auxSetting.m_dialNumber );
    combine( resultSetting.m_pinCode, auxSetting.m_pinCode );
    combine( resultSetting.m_pdpType, auxSetting.m_pdpType );

    return resultSetting;
}

SettingsProvider::Setting SettingsProvider::getTemplateSetting(
    const GSMModem::MobileNetworkInfo& mobileNetworkInfo ) const {
    auto setting = Setting();
    setting.m_dialNumber = "*99#";
    setting.m_operatorName = getUserFriendlyOperatorName( mobileNetworkInfo );

    return setting;
}

void SettingsProvider::constructProposalSettings() {
    m_settings.clear();

    const auto& simInfo = m_gsmModem.getSIMInfo();
    const auto mobileNetworkInfo = m_gsmModem.getMobileNetworkInfo();
    if( simInfo && mobileNetworkInfo ) {
        auto setting = combineSettings( getDatabaseUserSetting(), getTemplateSetting( mobileNetworkInfo ) );

        if( !setting.m_apnName.empty() ) {
            m_settings.push_back( setting );
        }
        else {
            for( const auto& apnName : getProposalAPNNames( simInfo, mobileNetworkInfo ) ) {
                setting.m_apnName = apnName;
                m_settings.push_back( setting );
            }
            for( const auto& apnName : m_configuration.m_defaultAPNSettings ) {
                setting.m_apnName = apnName;
                m_settings.push_back( setting );
            }
        }
    }
    else {
        LOGGER_DEBUG_RAW( cc::debug( "No SIM info, interface: " ) + cc::note( m_interfaceName ) );
    }

    showProposalSettings();
}

void SettingsProvider::showProposalSettings() const {
    auto js = JSON_EMPTY_ARR;
    for( const auto& setting : m_settings ) {
        js.push_back( setting.toJson() );
    }

    LOGGER_TRACE_RAW( cc::trace( "Proposal settings for mobile interface: " ) + cc::note( m_interfaceName ) +
                      cc::trace( ", " ) + cc::note( js.dump( 1 ) ) );
}

namespace {
/* In Canada, Hologram's mobile network partners do not provide an SPN name, so we see such networks
 * (which are Hologram ones) like a raw/physical networks which makes it impossible to differ such
 * networks from real raw/physical networks and makes it impossible to choose a correct APN name,
 * to handle this issue Hologram advised us to use the next approach... */
SettingsProvider::APNNames hologramCanadaHack( const GSMModem::SIMInfo& simInfo, GSMModem& modem ) {
    auto result = modem.executeCRSMCommand( 176, 28486, 0, 0, 17 );

    LOGGER_TRACE_RAW( cc::trace( "crsm 176,28486,0,0,17 command result - statusWord1: " ) +
                      cc::num10( result.m_statusWord1 ) + cc::trace( ", statusWord2: " ) +
                      cc::num10( result.m_statusWord2 ) + cc::trace( ", response: " ) + cc::note( result.m_response ) );

    if( result.m_valid && result.m_statusWord1 == 144 && result.m_statusWord2 == 0 ) {
        if( result.m_response == "01466C6F4C697665FFFFFFFFFFFFFFFFFF" ||
            result.m_response == "00486F6C6F6772616DFFFFFFFFFFFFFFFF" ) {
            return { "hologram" };
        }
    }

    if( !simInfo.m_iccid.compare( 0, 7, std::string( "8935711" ) ) ||
        !simInfo.m_iccid.compare( 0, 7, std::string( "8944500" ) ) ||
        !simInfo.m_iccid.compare( 0, 7, std::string( "8944501" ) ) ||
        !simInfo.m_iccid.compare( 0, 7, std::string( "8944502" ) ) ) {
        return { "hologram" };
    }

    return {};
}
}  // namespace

SettingsProvider::APNNames SettingsProvider::getProposalAPNNames(
    const GSMModem::SIMInfo& simInfo,
    const GSMModem::MobileNetworkInfo& mobileNetworkInfo ) const {
    if( !mobileNetworkInfo.m_spn.empty() ) {
        if( m_configuration.m_APNSettings.count( mobileNetworkInfo.m_spn ) ) {
            return m_configuration.m_APNSettings.at( mobileNetworkInfo.m_spn );
        }
    }

    const auto apnNames = hologramCanadaHack( simInfo, m_gsmModem );
    if( !apnNames.empty() ) {
        return apnNames;
    }

    const auto mobileNetworkId = mobileNetworkInfo.m_fnn + "\\" + mobileNetworkInfo.m_rplmn;

    if( m_configuration.m_APNSettings.count( mobileNetworkId ) ) {
        return m_configuration.m_APNSettings.at( mobileNetworkId );
    }

    if( m_configuration.m_APNSettings.count( mobileNetworkInfo.m_fnn ) ) {
        return m_configuration.m_APNSettings.at( mobileNetworkInfo.m_fnn );
    }

    LOGGER_DEBUG_RAW( cc::debug( "Failed to get APN names, ICCID: " ) + cc::note( simInfo.m_iccid ) +
                      cc::debug( ", IMSI: " ) + cc::note( simInfo.m_imsi ) + cc::debug( ", mobile network info: " ) +
                      cc::note( mobileNetworkInfo.toJson().dump() ) + cc::debug( ", interface: " ) +
                      cc::note( m_interfaceName ) );

    LOGGER_DEBUG_RAW( cc::debug( "Try to use default APN setting: " ) +
                      cc::json( Json( m_configuration.m_defaultAPNSettings ) ) );

    return {};
}

/* Long alphanumeric operator name|Service Provider Name[/Physical Carrier] */
std::string SettingsProvider::getUserFriendlyOperatorName(
    const GSMModem::MobileNetworkInfo& mobileNetworkInfo ) const {
    std::string operatorName;

    if( !mobileNetworkInfo ) {
        return "unknown";
    }

    using NumericOperatorName = std::string;
    using LongOperatorName = std::string;
    static const std::map< NumericOperatorName, LongOperatorName > numericOperatorNameToLongOperatorName = {
        { "311480", "Verizon Wireless" }, { "310410", "AT&T" },    { "310260", "T-Mobile" },
        { "25501", "MTS UKR" },           { "25503", "KYIVSTAR" }, { "25506", "lifecell" },
        { "00101", "Amarisoft Network" },
    };

    if( !mobileNetworkInfo.m_spn.empty() ) {
        operatorName = mobileNetworkInfo.m_spn;
    }
    else {
        operatorName = mobileNetworkInfo.m_fnn;
    }

    // append a physical carrier/operator name if a result operator name is ambiguous
    const auto it = numericOperatorNameToLongOperatorName.find( mobileNetworkInfo.m_rplmn );
    if( it != numericOperatorNameToLongOperatorName.end() ) {
        if( operatorName != it->second ) {
            operatorName += "/" + it->second;
        }
    }
    else {
        LOGGER_DEBUG_RAW( cc::debug( "Unknown numeric mobile operator name, interface: " ) +
                          cc::note( m_interfaceName ) );
    }

    return operatorName;
}

}  // namespace GSM
}  // namespace Network
}  // namespace HubPlatform
