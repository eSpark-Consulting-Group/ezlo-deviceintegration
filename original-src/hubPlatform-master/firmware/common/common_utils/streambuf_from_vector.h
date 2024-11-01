#pragma once
#include <streambuf>
#include <string>
#include <vector>

namespace HubPlatform {
namespace CommonUtils {

template < typename CharT, typename TraitsT = std::char_traits< CharT > >
class streambufFromVector : public std::basic_streambuf< CharT, TraitsT > {
public:
    explicit streambufFromVector( std::vector< CharT >& vec ) {
        this->setg( vec.data(), vec.data(), vec.data() + vec.size() );
    }
};

}  // namespace CommonUtils
}  // namespace HubPlatform