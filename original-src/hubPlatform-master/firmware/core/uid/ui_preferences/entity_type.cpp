#include "entity_type.h"

#include <unordered_map>

std::string entityTypeToStr( EntityType type ) {
    static std::unordered_map< EntityType, std::string, EntityTypeHash > entityTypeToStrMap = { { EntityType::ITEM,
                                                                                                  "item" } };

    return entityTypeToStrMap[ type ];
}

EntityType strToEntityType( const std::string& typeStr ) {
    static std::unordered_map< std::string, EntityType > strToEntityTypeMap = { { entityTypeToStr( EntityType::ITEM ),
                                                                                  EntityType::ITEM } };

    auto it = strToEntityTypeMap.find( typeStr );
    if( it != strToEntityTypeMap.end() )
        return it->second;

    return EntityType::INVALID;
}
