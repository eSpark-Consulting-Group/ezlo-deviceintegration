#pragma once

#include <json.hpp>  // Note: it is tiny/json.hpp

using Json = nlohmann::json;

using JsonObject = Json;
using JsonArray = Json;

namespace nlohmann {

inline void PrintTo( const Json& j, std::ostream* os )  // fixes crash of Google tests in case when jsons are not equal
{
    *os << j.dump();
}

}  // namespace nlohmann

class JsonBuilder;
