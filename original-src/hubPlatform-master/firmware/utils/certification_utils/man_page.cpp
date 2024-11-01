#include "man_page.h"

namespace HubPlatform {
namespace CertificationUtils {

void ManPage::section( const std::string& str ) {
    m_content << str << std::endl;
}

void ManPage::subSection( const std::string& str ) {
    m_content << m_shift << str << std::endl;
}

void ManPage::descr( const std::string& str ) {
    m_content << m_shift << m_shift << str << std::endl;
}

void ManPage::newLine() {
    m_content << std::endl;
}

std::stringstream& ManPage::content() {
    return m_content;
}

std::string ManPage::optional( const std::string& arg ) {
    return "[" + arg + "]";
}

std::string ManPage::join( const std::vector< std::string >& values ) {
    std::stringstream ss;
    auto it = values.cbegin();
    if( it != values.cend() )
        ss << *it++;
    for( ; it != values.cend(); ++it )
        ss << " " << *it;
    return ss.str();
}

}  // namespace CertificationUtils
}  // namespace HubPlatform
