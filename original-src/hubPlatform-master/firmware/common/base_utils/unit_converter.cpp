#include "unit_converter.h"
#include "json_utils/json_utils.h"

#include <set>
#include <cmath>
#include <tiny/tiny_utils.h>

const char UnitConverter::g_strCelsius[] = "°C";
const char UnitConverter::g_strFahrenheit[] = "°F";

static bool stat_is_stripped_unit_suffix_starts_with( const char* s, char c ) {
    if( s == nullptr || s[ 0 ] == '\0' )
        return false;
    std::string x( s );
    tiny::tools::replace_all( x, " ", "" );
    tiny::tools::replace_all( x, "\r", "" );
    tiny::tools::replace_all( x, "\n", "" );
    tiny::tools::replace_all( x, "\t", "" );
    tiny::tools::replace_all( x, "°", "" );
    x = tiny::tools::to_upper( x );
    if( x.empty() )
        return false;
    if( x[ 0 ] != c )
        return false;
    return true;
}
bool UnitConverter::is_celsius( const char* s ) {
    return stat_is_stripped_unit_suffix_starts_with( s, 'C' );
}
bool UnitConverter::is_celsius( const std::string& s ) {
    if( s.empty() )
        return false;
    return is_celsius( s.c_str() );
}
bool UnitConverter::is_fahrenheit( const char* s ) {
    return stat_is_stripped_unit_suffix_starts_with( s, 'F' );
}
bool UnitConverter::is_fahrenheit( const std::string& s ) {
    if( s.empty() )
        return false;
    return is_fahrenheit( s.c_str() );
}
std::string UnitConverter::beautify_temperature_suffix( const char* s ) {
    if( is_celsius( s ) )
        return common::CelsiusSuffix;
    if( is_fahrenheit( s ) )
        return common::FahrenheitSuffix;
    return s;
}
std::string UnitConverter::beautify_temperature_suffix( const std::string& s ) {
    if( s.empty() )
        return s;
    return beautify_temperature_suffix( s.c_str() );
}

bool UnitConverter::isTemperatureMethod( const std::string& methodName ) {
    const std::set< std::string > temperatureRelated = {
        "temp", "target_temp", "soil_temp", "water_temp", "thermostat_setpoint"
    };
    // check the methodName without leading "get_"
    return ( temperatureRelated.count( methodName.substr( 4, methodName.length() ) ) > 0 ) ? true : false;
}

std::string UnitConverter::getConvertedDegreesRuleTemplate( const std::string& ruleBlockTextTemplate,
                                                            const std::string& temperatureUnits ) {
    std::string ruleBlockTextTemplateCopy = ruleBlockTextTemplate;
    const std::string degreesPart = "°";  // was "&deg;"
    std::size_t foundAt = ruleBlockTextTemplateCopy.find( degreesPart );
    if( foundAt != std::string::npos )
        ruleBlockTextTemplateCopy.replace( foundAt, degreesPart.length() + 1, degreesPart + temperatureUnits );
    return ruleBlockTextTemplateCopy;
}

JsonObject UnitConverter::getCelsiusValueRuleField() {
    JsonObject valueField = JSON_EMPTY_OBJ;
    // similar for default F -20 130 70
    json_add_str( valueField, "name", "value" );
    json_add_str( valueField, "type", "number" );
    json_add_bool( valueField, "visibility", true );
    json_add_int( valueField, "minValue", -50 );
    json_add_int( valueField, "maxValue", 50 );
    json_add_int( valueField, "stepValue", 1 );
    json_add_int( valueField, "value", 20 );  // default value for C
    return valueField;
}

std::string UnitConverter::floatToFormattedString( const double& value ) {
    const char* precision = "%.4f";
    std::string result( 16, '\0' );
    /* value will be rounded with snprintf according to precision string */
    auto length = std::snprintf( (char*) result.c_str(), result.size(), precision, value );
    if( length >= 0 ) {
        result.resize( static_cast< size_t >( length ) );
        /* remove trailing zeros from result */
        auto last_nonzero = result.find_last_not_of( '0' );
        if( last_nonzero != result.find( '.' ) )
            last_nonzero++;
        result.erase( last_nonzero, std::string::npos );
    }

    return result;
}

std::string UnitConverter::rgbToFormattedString( uint8_t red, uint8_t green, uint8_t blue ) {
    std::ostringstream oss;
    oss << '#';
    oss.fill( '0' );
    oss.width( 6 );
    oss << std::uppercase << std::hex << ( ( red << 16 ) | ( green << 8 ) | blue );
    return oss.str();
}

std::string UnitConverter::getTemperatureUnits( const Json& args ) {
    return json_get_str( args, "temperatureUnits" );
}
