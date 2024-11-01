#include "extensions_manager/extensions/addon.h"
#include "extensions_manager/extensions/extensions_manager.h"
#include "extensions_manager/common/addon_utils.h"
#include "extensions_manager/em_messenger.h"

#include "log_utils/ezlo_log.h"
#include "database_client/idbclient.h"

#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace ExtensionsManager {

Addon::Addon( Environment& environment, const std::string& addonId )
: m_environment( environment )
, m_addonSummary( { addonId, CommonUtils::AddonVersion() } )
, m_setupId( 0 )
, m_status( Status::Stopped ) {}

Addon::~Addon() {
    teardown();
}

void Addon::setSummary( const CommonUtils::AddonSummary& addonSummary ) {
    m_addonSummary = addonSummary;
}

bool Addon::hasSummary() const {
    return m_addonSummary.isValid();
}

const CommonUtils::AddonSummary& Addon::getSummary() const {
    return m_addonSummary;
}

Addon::Status Addon::getStatus() const {
    return m_status;
}

bool Addon::startup( int64_t setupId ) {
    if( ( setupId != 0 ) && ( m_setupId == setupId ) ) {
        return false;
    }

    if( m_status == Status::Started ) {
        Log::info( cc::info( "Addon restarted: " ) + cc::note( getSummary().toString() ) );
    }
    else {
        Log::info( cc::info( "Addon started: " ) + cc::note( getSummary().toString() ) );
    }

    m_status = Status::Started;
    m_setupId = setupId;

    return true;
}

void Addon::teardown() {
    if( m_status != Status::Stopped ) {
        Log::info( cc::info( "Addon stopped: " ) + cc::note( getSummary().toString() ) );
    }
    m_status = Status::Stopped;
    m_setupId = 0;
}

Environment& Addon::getEnvironment() {
    return m_environment;
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
