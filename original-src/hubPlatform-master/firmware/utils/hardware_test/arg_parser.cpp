#include "hardware_test/arg_parser.h"

#include <map>
#include <exception>
#include <regex>

namespace HubPlatform {
namespace HardwareTest {
namespace {

struct Argument {
    std::string name;
    std::string alias;
    enum class Type { List, Value, Flag } type;
};

Argument ArgHelp{ "--help", "-h", Argument::Type::Flag };
Argument ArgExecOrder{ "--exec-order", "", Argument::Type::Value };
Argument ArgExclude{ "--exclude", "", Argument::Type::List };
Argument ArgSelect{ "--select", "", Argument::Type::List };
Argument ArgLedTimeout{ "--led-timeout", "", Argument::Type::Value };
Argument ArgSoundTimeout{ "--sound-timeout", "", Argument::Type::Value };
Argument ArgPingAddr{ "--ping-addr", "", Argument::Type::Value };
Argument ArgUniqueICCID{ "--unique-iccid", "", Argument::Type::Value };
Argument ArgCallbox{ "--callbox", "", Argument::Type::Value };
Argument ArgLTEBandRssiEdgeValues{ "--lte-band-rssi-edge-values", "", Argument::Type::List };
Argument ArgLTEGetRssiDelay{ "--lte-get-rssi-delay", "", Argument::Type::Value };

std::map< std::string, const Argument* > createArgMap() {
    std::map< std::string, const Argument* > map;
    auto extendMap = [&map]( const Argument& arg ) {
        if( !arg.name.empty() )
            map[ arg.name ] = &arg;
        if( !arg.alias.empty() )
            map[ arg.alias ] = &arg;
    };

    extendMap( ArgHelp );
    extendMap( ArgExecOrder );
    extendMap( ArgExclude );
    extendMap( ArgSelect );
    extendMap( ArgLedTimeout );
    extendMap( ArgSoundTimeout );
    extendMap( ArgPingAddr );
    extendMap( ArgUniqueICCID );
    extendMap( ArgCallbox );
    extendMap( ArgLTEBandRssiEdgeValues );
    extendMap( ArgLTEGetRssiDelay );

    return map;
}

auto ArgumentMap = createArgMap();

}  // namespace

namespace ArgUtils {

bool isNumber( const std::string& str ) {
    static const std::regex re( "\\d+" );
    return std::regex_match( str, re );
}

bool isIpAddr( const std::string& str ) {
    static const std::regex re( "(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})" );
    std::smatch match;
    if( !std::regex_match( str, match, re ) )
        return false;
    for( size_t i = 1; i <= 4; ++i )
        if( std::stoul( match[ i ] ) > 255 )
            return false;
    return true;
}

}  // namespace ArgUtils

ArgParser::ArgParser( const std::vector< std::string >& args, const std::vector< std::string >& knownTests )
: m_knownTests( knownTests ) {
    parseArgs( args );

    if( hasArg( "--help" ) ) {
        help();
        exit( 0 );
    }
}

void ArgParser::parseArgs( const std::vector< std::string >& args ) {
    auto splitArgValue = []( const std::string& s, std::string& name, std::string& value ) {
        size_t delimPos = s.find( '=' );
        name = s.substr( 0, delimPos );
        value = ( delimPos != std::string::npos ) ? s.substr( delimPos + 1 ) : "";
    };

    auto parseValueList = []( const std::string& valueList ) {
        std::vector< std::string > array;
        const char delim = ',';

        std::stringstream ss( valueList );
        std::string value;
        while( std::getline( ss, value, delim ) ) {
            array.emplace_back( value );
        }
        return array;
    };

    try {
        std::stringstream err_msg;
        for( auto& argNameValue : args ) {
            std::string argName;
            std::string argValue;
            splitArgValue( argNameValue, argName, argValue );

            auto argIt = ArgumentMap.find( argName );
            if( argIt == ArgumentMap.cend() ) {
                err_msg << "Argument \"" << argName << "\" not recognized" << std::endl;
                throw ArgumentError( err_msg.str() );
            }

            const Argument& argument = *( argIt->second );

            if( argument.type == Argument::Type::Flag ) {
                m_parsedArgs[ argument.name ];
                continue;
            }

            if( argument.type == Argument::Type::Value ) {
                m_parsedArgs[ argument.name ] = std::vector< std::string >{ argValue };
                continue;
            }

            if( argument.type == Argument::Type::List ) {
                auto values = parseValueList( argValue );
                if( values.empty() )
                    throw ArgumentError( "Argument \"" + argName + "\" has no values" );
                m_parsedArgs[ argument.name ] = values;
                continue;
            }
        }
    }
    catch( ArgumentError& e ) {
        std::cout << "Error! " << e.what() << std::endl << std::flush;
        help();
        exit( -1 );
    }
}

bool ArgParser::hasArg( const std::string& argName ) const {
    if( ArgumentMap.find( argName ) == ArgumentMap.cend() ) {
        std::cout << "Argument \"" << argName << "\" is not supported" << std::endl;
        exit( -1 );
    }
    return m_parsedArgs.find( argName ) != m_parsedArgs.cend();
}

std::string ArgParser::getValue( const std::string& argName ) const {
    auto mapIt = ArgumentMap.find( argName );
    if( mapIt == ArgumentMap.cend() )
        throw std::runtime_error( "Argument \"" + argName + "\" is not supported" );

    if( mapIt->second->type != Argument::Type::Value )
        throw std::runtime_error( "Value of argument \"" + argName + "\" has different type" );

    auto it = m_parsedArgs.find( argName );
    if( it == m_parsedArgs.cend() )
        throw std::runtime_error( "Value requested for non-existent argument: " + argName );
    if( it->second.empty() )
        throw std::runtime_error( "Value for argument \"" + argName + "\" is not specified" );
    return it->second.front();
}

std::vector< std::string > ArgParser::getArray( const std::string& argName ) const {
    auto mapIt = ArgumentMap.find( argName );
    if( mapIt == ArgumentMap.cend() )
        throw std::runtime_error( "Argument \"" + argName + "\" is not supported" );

    if( mapIt->second->type != Argument::Type::List )
        throw std::runtime_error( "Value of argument \"" + argName + "\" has different type" );

    auto it = m_parsedArgs.find( argName );
    if( it == m_parsedArgs.cend() )
        throw std::runtime_error( "Array requested for non-existent argument: " + argName );
    if( it->second.empty() )
        throw std::runtime_error( "Array of values for argument \"" + argName + "\" is not specified" );
    return it->second;
}

void ArgParser::help() const {
    const std::string sp4 = "    ";
    const std::string sp8 = "        ";
    std::stringstream ss;
    ss << "NAME" << std::endl;
    ss << sp4 << "hardware_test" << std::endl;
    ss << std::endl;

    ss << "SYNOPSIS" << std::endl;
    ss << sp4 << "hardware_test [options]" << std::endl;
    ss << sp4 << "hardware_test {-h | --help}" << std::endl;
    ss << sp4 << std::endl;

    ss << "OPTIONS" << std::endl;
    ss << sp4 << "--exec-order=parallel|serial" << std::endl;
    ss << sp8 << "Sets tests execution order" << std::endl;
    ss << std::endl;

    ss << sp4 << "--exclude=test1,test2,..." << std::endl;
    ss << sp8 << "Disables selected tests." << std::endl;
    ss << sp8 << "Note: options --exclude and --select are mutually exclusive" << std::endl;
    ss << std::endl;

    ss << sp4 << "--select=test1,test2,..." << std::endl;
    ss << sp8 << "Selects which tests to be run" << std::endl;
    ss << sp8 << "Note: options --exclude and --select are mutually exclusive" << std::endl;
    ss << std::endl;

    ss << sp4 << "--led-timeout=N" << std::endl;
    ss << sp8 << "Sets timeout in seconds of first LED test" << std::endl;
    ss << std::endl;

    ss << sp4 << "--sound-timeout=N" << std::endl;
    ss << sp8 << "Sets timeout in seconds to start sound test" << std::endl;
    ss << std::endl;

    ss << sp4 << "--ping-addr=<ip-addr>" << std::endl;
    ss << sp8 << "Sets IP address to ping" << std::endl;
    ss << std::endl;

    ss << sp4 << "--unique-iccid=true|false" << std::endl;
    ss << sp8 << "Sets if ICCID should be added to unique field" << std::endl;
    ss << std::endl;

    ss << sp4 << "--callbox=<ip-addr>" << std::endl;
    ss << sp8 << "Sets IP address of an LTE callbox" << std::endl;
    ss << std::endl;

    ss << sp4 << "--lte-band-rssi-edge-values=band_number:min_value:max_value,band_number:min_value:max_value,..."
       << std::endl;
    ss << sp8 << "Sets rssi edge values for each band" << std::endl;
    ss << std::endl;

    ss << sp4 << "--lte-get-rssi-delay=5" << std::endl;
    ss << sp8 << "Sets delay value (in seconds) to sleep before get an rssi value, default is 5s" << std::endl;
    ss << std::endl;

    ss << sp4 << "List of all tests: ";
    for( auto& test : m_knownTests )
        ss << test << ", ";
    ss << std::endl;

    std::cout << ss.str() << std::flush;
}

}  // namespace HardwareTest
}  // namespace HubPlatform
