#pragma once

#include <functional>
#include <string>

enum class EntityType { INVALID, ITEM };

struct EntityTypeHash {
    std::size_t operator()( EntityType entityType ) const {
        return std::hash< int >()( static_cast< int >( entityType ) );
    }
};

std::string entityTypeToStr( EntityType type );
EntityType strToEntityType( const std::string& typeStr );
