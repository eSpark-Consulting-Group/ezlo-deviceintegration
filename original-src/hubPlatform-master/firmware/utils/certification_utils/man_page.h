#pragma once

#include <string>
#include <sstream>
#include <vector>

namespace HubPlatform {
namespace CertificationUtils {

class ManPage {
public:
    void section( const std::string& str );
    void subSection( const std::string& str );
    void descr( const std::string& str );
    void newLine();
    std::stringstream& content();

    static std::string optional( const std::string& arg );
    static std::string join( const std::vector< std::string >& values );

private:
    const std::string m_shift = "    ";
    std::stringstream m_content;
};

}  // namespace CertificationUtils
}  // namespace HubPlatform
