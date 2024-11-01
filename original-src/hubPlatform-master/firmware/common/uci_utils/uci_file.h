#pragma once

#include "uci_utils/uci_settings.h"
#include <functional>

namespace HubPlatform {
namespace UciUtils {

class UciFile {
public:
    UciFile( std::string fileName );
    virtual ~UciFile() = default;

    const std::string& getFileName() const;

    const UciSection& findSection( const std::string& type, const std::string& name ) const;
    UciSection& findSection( const std::string& type, const std::string& name );
    bool removeSection( const std::string& type, const std::string& name );
    bool process( std::function< bool( UciSection& ) > action );

    bool hasSection( UciUtils::UciSection section );
    bool ensureSection( UciUtils::UciSection section );

    bool load();
    bool save();

protected:
    UciSettings::iterator getSection( const std::string& type, const std::string& name );
    UciSettings::const_iterator getSection( const std::string& type, const std::string& name ) const;

    UciSettings m_settings;

private:
    std::string m_fileName;
};

}  // namespace UciUtils
}  // namespace HubPlatform
