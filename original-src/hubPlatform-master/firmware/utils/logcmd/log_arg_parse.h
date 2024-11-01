#pragma once

#include <string>
#include <vector>

#include "json_utils/json_utils.h"

#include "log_filter.h"

namespace HubPlatform {
namespace LogCmd {

extern const char* const ARG_ALL_LOGS;
extern const char* const ARG_ROTATED_LOGS;
extern const char* const ARG_SELECTED_LOGS;
extern const char* const ARG_UPLOAD;
extern const char* const ARG_UPLOAD_ARCHIVED;
extern const char* const ARG_UPLOAD_POSTPONED;
extern const char* const ARG_ROTATE;
extern const char* const ARG_SLICE;
extern const char* const ARG_ROTATE_ON_SIZE_LIMIT;
extern const char* const ARG_STORE;
extern const char* const ARG_STORE_TO_POSTPONED;
extern const char* const ARG_UPDATE_SETTINGS;
extern const char* const ARG_AUTOREMOVE;
extern const char* const ARG_FORCE_UPLOAD;
extern const char* const ARG_OUTPUT_NAME;
extern const char* const ARG_ADD_TO_NAME;
extern const char* const ARG_OUTPUT_DIR;
extern const char* const ARG_CREATE_DIR;
extern const char* const ARG_LOG_DIR;
extern const char* const ARG_ROTATE_ARCHIVES;
extern const char* const ARG_NO_SYSTEM_LOGS;
extern const char* const ARG_IDLE;

using Args = std::vector< std::string >;

Json parseArgs( const Args& args );
void help();

// private:
namespace Impl {
using Key = std::string;
using Value = std::string;

void addFlag( Json& json, const Key& key, const Value& value );
void addValue( Json& json, const Key& key, const Value& value );
}  // namespace Impl

}  // namespace LogCmd
}  // namespace HubPlatform
