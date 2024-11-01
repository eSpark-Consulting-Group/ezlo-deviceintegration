#include "system_utils/crontab/crontab.h"

#include "system_utils/crontab/crontab_utils.h"
#include "system_utils/environment.h"

#include "common_utils/cpp14.h"
#include <sstream>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>

namespace HubPlatform {
namespace system_utils {

namespace {

constexpr std::chrono::milliseconds crontabLockTimeout{ 5000 };
constexpr std::chrono::milliseconds crontabLockInterval{ 100 };

std::string getCrontabPath() {
    return Environment::getSystemConfigsFolder() + "/crontabs/root";
}

std::string getCrontabLockPath() {
    return Environment::getTempDataFolder() + "/.crontab_lock";
}

}  // namespace

Crontab::Crontab( std::string filename )
: m_crontabLock( getCrontabLockPath() )
, m_filename( std::move( filename ) ) {}

Crontab::Crontab()
: Crontab( getCrontabPath() ) {}

void Crontab::addRecord( Record record ) {
    m_records.emplace_back( std::move( record ) );
    m_updated = true;
}

void Crontab::deleteRecord( const Command& command ) {
    auto it = findByCommand( command );
    if( it == m_records.end() )
        throw CrontabRecordError( "Crontab record not found" );
    m_records.erase( it );
    m_updated = true;
}

std::unique_ptr< Crontab::Record > Crontab::findRecord( const Command& command ) {
    auto it = findByCommand( command );
    if( it != m_records.end() ) {
        return std::make_unique< Crontab::Record >( *it );
    }
    return {};
}

void Crontab::updateCommandSchedule( const Command& command, Schedule schedule ) {
    auto it = findByCommand( command );
    if( it == m_records.end() )
        throw CrontabRecordError( "Crontab record not found" );

    if( it->first == schedule )
        return;

    m_updated = true;
    it->first = std::move( schedule );
}

std::list< Crontab::Record >::iterator Crontab::findByCommand( const Command& command ) {
    for( auto it = m_records.begin(); it != m_records.end(); ++it ) {
        if( it->second.find( command ) == 0 )
            return it;
    }
    return m_records.end();
}

void Crontab::load() {
    std::ifstream stream( m_filename );
    if( !stream.is_open() )
        throw CrontabError( "Crontab file not found" );

    std::string line;
    while( std::getline( stream, line ) ) {
        auto record = crontab_utils::parseLine( line );
        if( record )
            m_records.emplace_back( std::move( *record ) );
    }
}

void Crontab::write() {
    std::ofstream stream( m_filename );
    if( !stream.is_open() )
        throw CrontabError( "Crontab file not found" );

    for( auto& record : m_records ) {
        Crontab::Schedule& schedule = record.first;
        Crontab::Command& command = record.second;

        stream << schedule[ 0 ] << " " << schedule[ 1 ] << "    ";
        stream << schedule[ 2 ] << " " << schedule[ 3 ] << " " << schedule[ 4 ];

        stream << "   ";
        stream << command;
        stream << "\n";
    }
    stream.flush();
}

void Crontab::restartCron() {
#if !defined( __HUB_TYPE_LOCAL__ )
    std::system( "/etc/init.d/cron restart" );
#endif
}

bool Crontab::isUpdated() {
    return m_updated;
}

void Crontab::lock() {
    const auto start = std::chrono::steady_clock::now();
    while( std::chrono::steady_clock::now() - start < crontabLockTimeout ) {
        if( m_crontabLock.tryLock() )
            return;
        std::this_thread::sleep_for( crontabLockInterval );
    }
    throw CrontabError( "Unable to lock crontab" );
}

void Crontab::unlock() {
    m_crontabLock.unlock();
}

}  // namespace system_utils
}  // namespace HubPlatform
