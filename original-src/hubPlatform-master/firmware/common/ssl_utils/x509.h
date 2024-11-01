#pragma once

#include <string>
#include <vector>

namespace HubPlatform {
namespace SslUtils {

std::string asn1Now();

bool asn1TimeDiff( int& days, int& seconds, const std::string& lhs, const std::string& rhs );
bool asn1TimeLess( const std::string& lhs, const std::string& rhs );

std::pair< std::string, std::string > getValidityPeriod( const std::string& certificate );

using ValidityPeriods = std::vector< std::pair< std::string, std::string > >;
int chooseValidPeriod( const ValidityPeriods& periods, const std::string& reference = "" );

}  // namespace SslUtils
}  // namespace HubPlatform