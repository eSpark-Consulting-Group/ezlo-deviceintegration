#include "migration_tool/system_repair/system_repair_manager.h"
#include "migration_tool/system_repair/partmap_comparison_fix.h"
#include "migration_tool/system_repair/hostname_fix.h"
#include "migration_tool/system_repair/timezone_fix.h"
#include "migration_tool/system_repair/firewall_fix.h"
#include "migration_tool/system_repair/dhcp_fix.h"

#include "tiny/tiny_consolecolors.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"

namespace HubPlatform {
namespace MigrationTool {

SystemRepairManager::SystemRepairManager( IDbClient& dbClient )
: m_dbClient( dbClient ) {
    m_systemFixes.push_back( std::make_unique< HostnameFix >() );
    m_systemFixes.push_back( std::make_unique< TimezoneFix >( m_dbClient ) );
    m_systemFixes.push_back( std::make_unique< PartmapComparisonFix >() );
    m_systemFixes.push_back( std::make_unique< FirewallFix >() );
    m_systemFixes.push_back( std::make_unique< DhcpFix >() );
}

bool SystemRepairManager::repair() {
    bool result = true;

    for( auto& fix : m_systemFixes ) {
        Log::debug( cc::debug( "Checking if " ) + cc::note( fix->getName() ) + cc::debug( " required" ) );
        if( !fix->shouldApply() ) {
            Log::debug( cc::debug( "Looks like " ) + cc::note( fix->getName() ) + cc::debug( " is not needed" ) );
            continue;
        }

        Log::info( cc::info( "Applying " ) + cc::note( fix->getName() ) );
        if( !fix->apply() ) {
            result = false;
            Log::error( cc::error( "Failed to apply " ) + cc::note( fix->getName() ) );
        }
        Log::debug( cc::debug( "Successfully applied " ) + cc::note( fix->getName() ) );
    }
    return result;
}

}  // namespace MigrationTool
}  // namespace HubPlatform
