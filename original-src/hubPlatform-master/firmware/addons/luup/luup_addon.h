#pragma once

#include "addon_interfaces/addon.h"

#include <string>
#include <vector>
#include <memory>

class BaseService;

namespace HubPlatform {
namespace Addons {

class LuupAddon final : public Addon {
public:
    static const char* const Name;

public:
    LuupAddon( const std::string& webRoot );
    ~LuupAddon();

    CommonUtils::AddonVersion getVersion() const override;

protected:
    void setUp() override;

private:
    const std::string m_webRoot;
    std::vector< std::unique_ptr< BaseService > > m_services;
};

}  // namespace Addons
}  // namespace HubPlatform
