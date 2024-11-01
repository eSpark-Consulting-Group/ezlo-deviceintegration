#include "scenes_manager/scenes_engine/item_cache/item_cache_resources.h"
#include "database_client/db_tables_names.h"

namespace SceneEngine {
namespace Resources {

namespace DbNames {
const char* const ItemCacheDbName = HaScenesItemCacheDBName;
}

namespace Fields {
const char controllerId[] = "controllerId";
const char itemId[] = "itemId";
const char id[] = "_id";
}  // namespace Fields
}  // namespace Resources
}  // namespace SceneEngine
