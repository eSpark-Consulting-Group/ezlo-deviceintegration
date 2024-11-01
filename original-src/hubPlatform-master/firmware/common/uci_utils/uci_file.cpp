#include "uci_utils/uci_file.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

namespace HubPlatform {
namespace UciUtils {

UciFile::UciFile( std::string fileName )
: m_fileName( std::move( fileName ) ) {}

bool UciFile::load() {
    if( !readSettings( m_fileName, m_settings ) ) {
        Log::error( cc::note( m_fileName ) + cc::error( " has unsupported format" ) );
        return false;
    }
    return true;
}

bool UciFile::save() {
    if( !writeSettings( m_fileName, m_settings ) ) {
        Log::error( cc::error( "Error saving " ) + cc::note( m_fileName ) );
        return false;
    }
    return true;
}

const std::string& UciFile::getFileName() const {
    return m_fileName;
}

const UciSection& UciFile::findSection( const std::string& type, const std::string& name ) const {
    auto existingIt = getSection( type, name );
    if( existingIt == m_settings.end() ) {
        static const UciSection invalidSection;
        return invalidSection;
    }
    return *existingIt;
}

UciSection& UciFile::findSection( const std::string& type, const std::string& name ) {
    auto existingIt = getSection( type, name );
    if( existingIt == m_settings.end() ) {
        static UciSection invalidSection;
        invalidSection = {};
        return invalidSection;
    }
    return *existingIt;
}

bool UciFile::process( std::function< bool( UciSection& ) > action ) {
    for( auto& section : m_settings ) {
        if( !action( section ) )
            return false;
    }
    return true;
}

bool UciFile::removeSection( const std::string& type, const std::string& name ) {
    auto existingIt = getSection( type, name );
    if( existingIt == m_settings.end() ) {
        return false;
    }
    m_settings.erase( existingIt );
    return true;
}

bool UciFile::hasSection( UciUtils::UciSection section ) {
    auto existingIt = getSection( section.m_type, section.m_name );
    if( existingIt == m_settings.end() ) {
        return false;
    }

    return ( section == *existingIt );
}

bool UciFile::ensureSection( UciUtils::UciSection section ) {
    auto existingIt = getSection( section.m_type, section.m_name );
    if( existingIt == m_settings.end() ) {
        // Section is missing, add it
        m_settings.push_back( std::move( section ) );
        return true;
    }

    if( section != *existingIt ) {
        // Section differs from expected, modify it
        *existingIt = std::move( section );
        return true;
    }

    // Everything is as expected, nothing to do
    return false;
}

UciSettings::iterator UciFile::getSection( const std::string& type, const std::string& name ) {
    auto sectionIt = std::find_if( m_settings.begin(), m_settings.end(), [&]( const UciSection& section ) {
        return ( type.empty() || section.m_type == type ) && ( name.empty() || section.m_name == name );
    } );
    return sectionIt;
}

UciSettings::const_iterator UciFile::getSection( const std::string& type, const std::string& name ) const {
    auto sectionIt = std::find_if( m_settings.begin(), m_settings.end(), [&]( const UciSection& section ) {
        return ( type.empty() || section.m_type == type ) && ( name.empty() || section.m_name == name );
    } );
    return sectionIt;
}

}  // namespace UciUtils
}  // namespace HubPlatform
