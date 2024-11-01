#pragma once

#include <string>

namespace HubPlatform {
namespace Version {

extern const char HaBranch[];
extern const char HaCommit[];
extern const char HaBuildDate[];
extern const char HaBuilder[];

std::string git_version();
std::string ha_version();

}  // namespace Version
}  // namespace HubPlatform
