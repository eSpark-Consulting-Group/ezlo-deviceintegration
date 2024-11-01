#pragma once

#include "system_utils/timezone/timezone.h"

namespace HubPlatform {
namespace system_utils {

class OpenWrtTimeZone : public TimeZone {
public:
    virtual void setTimeZone( const std::string& zoneName ) override;
    virtual std::string getTimeZone() override;
    virtual bool isTimeZoneDefault() override;
};

}  // namespace system_utils
}  // namespace HubPlatform
