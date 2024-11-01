#include "uci_utils/uci_settings.h"
#include "base_utils/utils.h"

#include <cassert>
#include <fstream>

namespace HubPlatform {
namespace UciUtils {

namespace {
constexpr const char ConfigPrefix[] = "config";
constexpr const char OptionPrefix[] = "option";
constexpr const char ListPrefix[] = "list";
constexpr const char PackagePrefix[] = "package";

template < typename It >
size_t skipLineSpaces( It& it, It end ) {
    size_t result = 0;
    for( ; it != end && ( *it == ' ' || *it == '\t' ); ++it, ++result )
        ;
    return result;
}

template < typename It >
size_t skipComment( It& it, It end ) {
    size_t result = 0;
    if( it != end && *it == '#' ) {
        do {
            ++it;
            ++result;
        } while( it != end && *it != '\0' && *it != '\n' );
    }
    return result;
}

template < typename It, typename OutputIt >
size_t readEscapedOrQuotedString( It& it, It end, OutputIt outputIt ) {
    bool escaped = false;
    char quote = '\0';
    size_t result = 0;
    for( ; it != end; ++it, ++result ) {
        const auto symbol = *it;

        if( symbol == 0 ) {
            break;
        }
        else if( escaped ) {
            assert( !quote );
            escaped = false;

            *( outputIt++ ) = symbol;
        }
        else if( quote ) {
            if( quote == symbol ) {
                quote = 0;
            }
            else {
                *( outputIt++ ) = symbol;
            }
        }
        else {
            if( isspace( static_cast< unsigned char >( symbol ) ) || symbol == '#' ) {
                break;
            }
            else if( symbol == '\\' ) {
                escaped = true;
            }
            else if( symbol == '\'' || symbol == '"' ) {
                quote = symbol;
            }
            else {
                *( outputIt++ ) = symbol;
            }
        }
    }

    return result;
}

template < typename It, typename Handler >
bool forEachLine( It it, const It end, Handler&& lineHandler ) {
    constexpr const size_t MaxStringsCount = 3u;

    std::array< std::string, MaxStringsCount > strings;

    auto currentString = strings.begin();

    while( true ) {
        skipLineSpaces( it, end );
        skipComment( it, end );

        const auto symbol = it == end ? '\0' : *it;

        if( isspace( static_cast< unsigned char >( symbol ) ) || symbol == '\0' ) {
            if( currentString != strings.begin() ) {
                if( !lineHandler( std::move( strings[ 0 ] ), std::move( strings[ 1 ] ), std::move( strings[ 2 ] ) ) ) {
                    return false;
                }

                do {
                    ( --currentString )->clear();
                } while( currentString != strings.begin() );
            }

            if( it == end ) {
                return true;
            }
            ++it;
        }
        else {
            if( currentString == strings.end() ) {
                return false;
            }

            if( readEscapedOrQuotedString( it, end, std::back_inserter( *currentString ) ) == 0 ) {
                assert( false );
                return false;
            }
            ++currentString;
        }
    }
}

}  // namespace

bool operator==( const UciSection& first, const UciSection& second ) {
    return first.m_type == second.m_type && first.m_name == second.m_name &&
           first.m_listedOptions == second.m_listedOptions && first.m_singleOptions == second.m_singleOptions;
}

bool operator!=( const UciSection& first, const UciSection& second ) {
    return !( first == second );
}

std::ostream& operator<<( std::ostream& stream, const UciSection& section ) {
    auto escapeString = []( const std::string& str ) { return escapeSymbolsByFirst( str, "\\'\" \t\n\f\r\v#" ); };

    auto isKeyValid = []( const std::string& key ) {
        return !key.empty() && std::all_of( key.begin(), key.end(), []( unsigned char c ) {
            return !std::isspace( c ) && c != '\'' && c != '"' && c != '\\';
        } );
    };

    auto writeOptionWithPrefix = [&]( const char* prefix, const std::pair< std::string, std::string >& option ) {
        if( isKeyValid( option.first ) ) {
            const auto value = option.second.empty() ? std::string( "''" ) : escapeString( option.second );
            stream << '\t' << prefix << ' ' << option.first << ' ' << value << '\n';
        }
    };

    if( isKeyValid( section.m_type ) ) {
        stream << ConfigPrefix << ' ' << section.m_type;

        // name rules are like key rules
        if( isKeyValid( section.m_name ) ) {
            stream << ' ' << escapeString( section.m_name );
        }

        stream << '\n';

        for( const auto& option : section.m_singleOptions ) {
            writeOptionWithPrefix( OptionPrefix, option );
        }

        for( const auto& option : section.m_listedOptions ) {
            writeOptionWithPrefix( ListPrefix, option );
        }

        stream << '\n';
    }

    return stream;
}

std::ostream& operator<<( std::ostream& stream, const UciSettings& settings ) {
    for( const auto& section : settings ) {
        stream << section;
    }

    return stream;
}

std::istream& operator>>( std::istream& stream, UciSettings& outputSettings ) {
    UciSettings settings;
    bool canAddOptions = false;

    auto onLineFound = [&settings, &canAddOptions](
                           std::string lineType, std::string lineName, std::string lineValue ) {
        if( lineType.empty() || lineName.empty() ) {
            return false;
        }

        if( lineType == ConfigPrefix ) {
            settings.push_back( UciSection{ std::move( lineName ), std::move( lineValue ), {}, {} } );
            canAddOptions = true;
            return true;
        }

        if( lineType == OptionPrefix ) {
            if( !canAddOptions ) {
                return false;
            }

            assert( !settings.empty() );
            settings.back().m_singleOptions.emplace( std::move( lineName ), std::move( lineValue ) );
        }
        else if( lineType == ListPrefix ) {
            if( !canAddOptions ) {
                return false;
            }

            assert( !settings.empty() );
            settings.back().m_listedOptions.emplace( std::move( lineName ), std::move( lineValue ) );
        }
        else if( lineType == PackagePrefix ) {
            canAddOptions = false;
        }
        else {
            return false;
        }

        return true;
    };

    if( forEachLine( std::istreambuf_iterator< char >( stream ), std::istreambuf_iterator< char >(), onLineFound ) ) {
        outputSettings = std::move( settings );
    }
    else {
        stream.setstate( std::istream::failbit );
    }

    return stream;
}

bool readSettings( const std::string& fileName, UciUtils::UciSettings& settings ) {
    std::ifstream stream( fileName );
    if( !stream.is_open() )
        return false;

    stream >> settings;

    if( !stream.good() ) {
        return false;
    }

    return true;
}

bool writeSettings( const std::string& fileName, const UciUtils::UciSettings& settings ) {
    std::ofstream stream( fileName );
    if( !stream.is_open() )
        return false;

    stream << settings;
    stream.flush();
    return true;
}

}  // namespace UciUtils
}  // namespace HubPlatform
