#ifndef RULEBLOCKS_H
#define RULEBLOCKS_H

#include <string>

#include "json_utils/json_wrapper.h"

namespace RuleBlocks {
///@param[in] parentId Non-empty to query only dataitems with such parent id or empty to query *only* parentless
/// dataitems
JsonArray getList( const JsonObject& params,
                   const std::string& blockType,
                   const std::string& conditionFilter,
                   const std::string& pageId,
                   const bool toItems );

size_t getBlocksCount( const std::string& blockType, const std::string& parentId );

}  // namespace RuleBlocks

#endif  // RULEBLOCKS_H
