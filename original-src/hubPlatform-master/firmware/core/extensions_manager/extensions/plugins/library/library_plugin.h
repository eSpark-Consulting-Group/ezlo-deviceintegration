#pragma once

#include "extensions_manager/extensions/plugins/plugin.h"
#include "extensions_manager/extensions/plugins/library/library_config.h"

#include <memory>

namespace HubPlatform {
namespace ExtensionsManager {

class Environment;
class BaseConfig;

class LibraryPlugin final : public Plugin {
public:
    LibraryPlugin( Environment& environment,
                   std::unique_ptr< BaseConfig > baseConfigInfo,
                   std::unique_ptr< LibraryConfig > libraryConfigInfo );

    void startup() override;
    bool uninstall() override;

private:
    std::unique_ptr< LibraryConfig > m_libraryConfigInfo;
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
