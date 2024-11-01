#pragma once

#include "json_utils/json_wrapper.h"
#include "base_utils/suffixes.h"

#include <string>

class UnitConverter {
public:
    static const char g_strCelsius[];
    static const char g_strFahrenheit[];
    static bool is_celsius( const char* s );
    static bool is_celsius( const std::string& s );
    static bool is_fahrenheit( const char* s );
    static bool is_fahrenheit( const std::string& s );
    static std::string beautify_temperature_suffix( const char* s );
    static std::string beautify_temperature_suffix( const std::string& s );
    static bool isTemperatureMethod( const std::string& methodName );
    static std::string getConvertedDegreesRuleTemplate( const std::string& ruleBlockTextTemplate,
                                                        const std::string& temperatureUnits );
    static JsonObject getCelsiusValueRuleField();
    static std::string floatToFormattedString( const double& value );
    static std::string rgbToFormattedString( uint8_t red, uint8_t green, uint8_t blue );

private:
    static std::string getTemperatureUnits( const Json& args );
};
