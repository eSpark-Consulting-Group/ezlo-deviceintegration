#pragma once

#include "common_utils/addons/addon_summary.h"

namespace HubPlatform {
namespace ExtensionsManager {

class Environment;

class Addon final {
public:
    enum class Status { Stopped, Started };

public:
    Addon( Environment& environment, const std::string& addonId );
    ~Addon();

    void setSummary( const CommonUtils::AddonSummary& addonSummary );
    bool hasSummary() const;
    const CommonUtils::AddonSummary& getSummary() const;

    Status getStatus() const;

    bool startup( int64_t setupId );
    void teardown();

private:
    Environment& getEnvironment();

private:
    Environment& m_environment;
    CommonUtils::AddonSummary m_addonSummary;
    int64_t m_setupId;
    Status m_status;
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
