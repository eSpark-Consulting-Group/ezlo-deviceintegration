#pragma once

#include "system_utils/crontab/crontab.h"

namespace HubPlatform {
namespace system_utils {
namespace crontab_utils {

std::unique_ptr< Crontab::Record > parseLine( const std::string& line );

}  // namespace crontab_utils
}  // namespace system_utils
}  // namespace HubPlatform
