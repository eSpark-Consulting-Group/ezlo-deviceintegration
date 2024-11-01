#pragma once

#include "networkd/interfaces/nm_interface_settings.h"

#include <string>

namespace HubPlatform {
namespace Network {

class APInterfaceSettings final : public InterfaceSettings {
public:
    struct APSettings {
        std::string m_apName;

        bool m_isEmpty = true;

        bool empty() const;
    };

    APInterfaceSettings( IDbClient& dbClient, std::string interfaceName, EnablePolicy defaultEnablePolicy );

    static APSettings getAPSettings( Json customSettings );

    APSettings getAPSettings() const;
    bool setAPSettings( APSettings apSettings );

private:
    bool checkAPSettings( const APSettings& apSettings );
};

}  // namespace Network
}  // namespace HubPlatform
