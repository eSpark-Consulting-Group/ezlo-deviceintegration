#include "config_reader/config_reader.h"

#include "system_utils/environment.h"

#include "common_utils/cpp14.h"
#include "json_utils/json_fwd.h"
#include "tiny/include/yamc/yamc_shared_lock.hpp"
#include "tiny/include/yamc/alternate_shared_mutex.hpp"

#include "tiny/include/tiny/tiny_utils.h"

#include <iterator>
#include <cassert>
#include <unordered_map>
#include <cstring>

namespace HubPlatform {

namespace {

constexpr const char ConfigFileExtension[] = ".cfg";
constexpr const char PathSeparator = '/';

constexpr const char VariableOpener[] = "${";
constexpr const char VariableCloser = '}';

const char* skipFirstPathSeparator( const char* path ) {
    assert( path );
    return path + ( *path == PathSeparator );
}

std::string getVariableValue( const std::string& variableName ) {
    static const std::map< std::string, std::string ( * )() > valueGetters = {
        { "firmware_root", Environment::getFirmwareRootFolder },
        { "firmware_bin", Environment::getBinariesFolder },
        { "firmware_etc", Environment::getConfigsFolder }
    };

    const auto itFind = valueGetters.find( variableName );
    return itFind == valueGetters.end() ? std::string() : itFind->second();
}

bool variablesProcessor( int depth, Json::parse_event_t event, Json& parsed ) {
    if( ( event != Json::parse_event_t::value ) || ( !parsed.is_string() ) ) {
        return true;
    }

    auto& resultString = *parsed.get< std::string* >();
    size_t lastFindPosition = 0;

    do {
        auto variableBegin = resultString.find( VariableOpener, lastFindPosition );
        if( variableBegin == std::string::npos ) {
            return true;
        }

        const auto actualVariableBegin = variableBegin + strlen( VariableOpener );
        auto variableEnd = resultString.find( VariableCloser, variableBegin );
        if( ( variableEnd == std::string::npos ) || ( ( variableEnd - variableBegin ) == 0 ) ) {
            lastFindPosition = actualVariableBegin;
            continue;
        }

        const auto variableName = resultString.substr( actualVariableBegin, variableEnd - actualVariableBegin );
        const auto variableValue = getVariableValue( variableName );
        if( variableValue.empty() ) {
            lastFindPosition = variableEnd - variableBegin + 1;
        }
        else {
            resultString.replace( variableBegin, variableEnd - variableBegin + 1, getVariableValue( variableName ) );
        }

    } while( true );

    return true;
}

}  // namespace

class ConfigReader::Impl {
public:
    Impl( SourceReader sourceReader, SpecialSourceReaders specialSourceReaders )
    : m_sourceReader( std::move( sourceReader ) )
    , m_specialSourceReaders( std::move( specialSourceReaders ) ) {}

    template < typename Type >
    static std::enable_if_t< !std::is_reference< Type >::value, Type > get( const JsonObject& json ) {
        try {
            return json.get< Type >();
        }
        catch( const std::exception& error ) {
            throw ReadingError( error.what() );
        }
    }

    // Implementation for references
    template < typename Type >
    static std::enable_if_t< std::is_reference< Type >::value, Type > get( const JsonObject& json ) {
        if( auto result = get< typename std::add_pointer< Type >::type >( json ) ) {
            return *result;
        }

        throw ReadingError( "Invalid type" );
    }

    template < typename Type >
    Type get( const char* path ) const {
        return get< Type >( getJsonByPath( skipFirstPathSeparator( path ) ) );
    }

    static std::vector< std::string > getStringArray( const JsonObject& json ) {
        if( json.is_array() ) {
            std::vector< std::string > vec = json;
            return vec;
        }

        throw ReadingError( "Invalid type" );
    }

    static std::vector< JsonObject > getJsonArray( const JsonObject& json ) {
        if( json.is_array() ) {
            std::vector< JsonObject > vec = json;
            return vec;
        }

        throw ReadingError( "Invalid type" );
    }

    static std::vector< std::string > getObjectKeys( const JsonObject& json ) {
        if( json.is_object() ) {
            std::vector< std::string > vec;

            for( auto it = json.cbegin(); it != json.cend(); it++ ) {
                vec.push_back( it.key() );
            }

            return vec;
        }

        throw ReadingError( "Invalid type" );
    }

    static size_t getArraySize( const JsonObject& json ) {
        if( json.is_array() ) {
            return json.size();
        }

        throw ReadingError( "Invalid type" );
    }

    static bool has( const JsonObject& json ) noexcept {
        return json.is_number() || json.is_boolean() || json.is_string() || json.is_array();
    }

    bool has( const char* path ) const noexcept {
        try {
            if( auto json = getJsonPtrByPath( skipFirstPathSeparator( path ) ) ) {
                return has( *json );
            }
        }
        catch( const std::exception& ) {
        }

        return false;
    }

    const JsonObject* getJsonPtrByPath( const char* path, const JsonObject* currentJsonObject = nullptr ) const {
        assert( path );

        try {
            for( const auto* keyBegin = path;; ++path ) {
                if( *path == PathSeparator || *path == '\0' ) {
                    if( currentJsonObject ) {
                        auto itFind = currentJsonObject->find( std::string( keyBegin, path ) );
                        if( itFind == currentJsonObject->end() ) {
                            return nullptr;
                        }

                        currentJsonObject = &( *itFind );
                    }
                    else {
                        currentJsonObject = &getJsonBySource( { keyBegin, path } );
                    }

                    if( *path == '\0' ) {
                        break;
                    }

                    keyBegin = path + 1;
                }
            }
        }
        catch( const std::exception& error ) {
            return nullptr;
        }

        return ( !currentJsonObject || currentJsonObject->is_null() ) ? nullptr : currentJsonObject;
    }

    const JsonObject& getJsonByPath( const char* path, const JsonObject* currentJsonObject = nullptr ) const {
        assert( path );

        try {
            for( const auto* keyBegin = path;; ++path ) {
                if( *path == PathSeparator || *path == '\0' ) {
                    currentJsonObject = currentJsonObject ?
                                            &( *currentJsonObject ).at( std::string( keyBegin, path ) ) :
                                            &getJsonBySource( { keyBegin, path } );

                    if( *path == '\0' ) {
                        break;
                    }

                    keyBegin = path + 1;
                }
            }
        }
        catch( const std::exception& error ) {
            throw ReadingError( error.what() );
        }

        if( !currentJsonObject || currentJsonObject->is_null() ) {
            throw ReadingError( "Invalid source" );
        }

        return *currentJsonObject;
    }

    const JsonObject& getJsonByPath( const size_t index, const JsonObject* currentJsonObject ) const {
        if( !currentJsonObject ) {
            throw ReadingError( "Index cannot be a source" );
        }

        try {
            return currentJsonObject->at( index );
        }
        catch( const std::exception& error ) {
            throw ReadingError( error.what() );
        }
    }

private:
    using SharedMutex = yamc::alternate::shared_mutex;

    // gets json object with shared locking or creates and gets object with full locking
    const JsonObject& getJsonBySource( std::string&& source ) const {
        {
            yamc::shared_lock< SharedMutex > sharedLocker( m_cacheMutex );
            auto it = m_jsonCache.find( source );

            if( it != m_jsonCache.end() ) {
                return it->second;
            }
        }

        return getJsonBySourceWithReadWriteLock( std::move( source ) );
    }

    const SourceReader& chooseSourceReader( const std::string& source ) const {
        auto it = m_specialSourceReaders.find( source );
        return ( it != m_specialSourceReaders.end() ) ? it->second : m_sourceReader;
    }

    const JsonObject& getJsonBySourceWithReadWriteLock( std::string&& source ) const {
        std::unique_lock< SharedMutex > locker( m_cacheMutex );

        const auto insertRes = m_jsonCache.emplace( std::move( source ), JsonObject() );

        // double check if called from getJsonBySource
        if( insertRes.second ) {
            try {
                auto& chosenSourceReader = chooseSourceReader( insertRes.first->first );

                if( const auto stream = chosenSourceReader( insertRes.first->first ) ) {
                    // TODO: json::parse fails in special case when stream is used
                    // temporary workaround. Should be reported as the bug !!!
                    std::string value( std::istreambuf_iterator< char >( *stream ), {} );
                    insertRes.first->second = Json::parse( value, variablesProcessor );
                }
            }
            catch( const std::exception& ex ) {
                throw ReadingError( "Invalid source or json" );
            }
        }

        return insertRes.first->second;
    }

private:
    SourceReader m_sourceReader;
    SpecialSourceReaders m_specialSourceReaders;

    mutable std::unordered_map< std::string, JsonObject > m_jsonCache;
    mutable SharedMutex m_cacheMutex;
};

class ConfigReader::Path::Impl {
public:
    Impl( const ConfigReader& configReader )
    : m_configReaderImpl( configReader.m_impl ) {}

    Impl( const Impl& ) noexcept = default;

    template < typename Path >
    void append( const Path& path ) {
        m_json = &m_configReaderImpl->getJsonByPath( path, m_json );
    }

    const JsonObject& getJson() const {
        if( isEmpty() ) {
            throw ReadingError( "Path is empty!" );
        }

        return *m_json;
    }

    bool isEmpty() const noexcept {
        return m_json == nullptr;
    }

private:
    ConfigReader::ImplPtr m_configReaderImpl;
    const JsonObject* m_json = nullptr;
};

ConfigReader::Path::Path( const ConfigReader& configReader, const char* path )
: m_impl( std::make_unique< Impl >( configReader ) ) {
    assert( path );
    if( *path ) {
        m_impl->append( skipFirstPathSeparator( path ) );
    }
}

ConfigReader::Path::Path( Path&& ) noexcept = default;
ConfigReader::Path& ConfigReader::Path::operator=( Path&& ) noexcept = default;

ConfigReader::Path::Path( const Path& other ) noexcept
: m_impl( std::make_unique< Impl >( *other.m_impl ) ) {}

ConfigReader::Path& ConfigReader::Path::operator=( const Path& other ) noexcept {
    m_impl = std::make_unique< Impl >( *other.m_impl );
    return *this;
}

ConfigReader::Path::~Path() = default;

ConfigReader::Path ConfigReader::Path::operator/( const char* subPath ) const& {
    return Path( *this ) / subPath;
}
ConfigReader::Path ConfigReader::Path::operator/( const char* subPath ) && {
    m_impl->append( subPath );
    return std::move( *this );
}

ConfigReader::Path ConfigReader::Path::operator/( size_t index ) const& {
    return Path( *this ) / index;
}

ConfigReader::Path ConfigReader::Path::operator/( size_t index ) && {
    m_impl->append( index );
    return std::move( *this );
}

const ConfigReader::Path::Impl& ConfigReader::Path::getImpl( PrivateTag ) const noexcept {
    return *m_impl;
}

ConfigReader::ConfigReader()
: ConfigReader(
      []( const std::string& sourceName ) {
          auto stream = std::make_unique< std::ifstream >();
          auto fileName = Environment::getConfigsFolder() + "/" + sourceName + ConfigFileExtension;
          stream->open( fileName );

          if( !stream->is_open() ) {
              stream.reset();
          }

          return StreamUPtr( stream.release() );
      },
      {} ) {}

ConfigReader::ConfigReader( SourceReader sourceReader, SpecialSourceReaders specialSourceReaders )
: m_impl( std::make_shared< Impl >( std::move( sourceReader ), std::move( specialSourceReaders ) ) ) {}

int ConfigReader::getInt( const char* path ) const {
    return m_impl->get< int >( path );
}

int ConfigReader::getInt( const Path& path ) {
    return Impl::get< int >( path.getImpl( {} ).getJson() );
}

float ConfigReader::getFloat( const char* path ) const {
    return m_impl->get< float >( path );
}

float ConfigReader::getFloat( const Path& path ) {
    return Impl::get< float >( path.getImpl( {} ).getJson() );
}

bool ConfigReader::getBool( const char* path ) const {
    return m_impl->get< bool >( path );
}

bool ConfigReader::getBool( const Path& path ) {
    return Impl::get< bool >( path.getImpl( {} ).getJson() );
}

const std::string& ConfigReader::getString( const char* path ) const {
    return m_impl->get< const std::string& >( path );
}

const std::string& ConfigReader::getString( const Path& path ) {
    return Impl::get< const std::string& >( path.getImpl( {} ).getJson() );
}

bool ConfigReader::hasValue( const char* path ) const noexcept {
    return m_impl->has( path );
}

bool ConfigReader::hasValue( const Path& path ) noexcept {
    const auto& pathImpl = path.getImpl( {} );
    return !pathImpl.isEmpty() && Impl::has( pathImpl.getJson() );
}

size_t ConfigReader::getArraySize( const Path& path ) {
    return Impl::getArraySize( path.getImpl( {} ).getJson() );
}

std::vector< std::string > ConfigReader::getStringArray( const Path& path ) {
    return Impl::getStringArray( path.getImpl( {} ).getJson() );
}

std::vector< JsonObject > ConfigReader::getJsonArray( const Path& path ) {
    return Impl::getJsonArray( path.getImpl( {} ).getJson() );
}

std::vector< std::string > ConfigReader::getObjectKeys( const Path& path ) {
    return Impl::getObjectKeys( path.getImpl( {} ).getJson() );
}

JsonObject ConfigReader::getJsonObject( const ConfigReader::Path& path ) {
    return path.getImpl( {} ).getJson();
}

}  // namespace HubPlatform
