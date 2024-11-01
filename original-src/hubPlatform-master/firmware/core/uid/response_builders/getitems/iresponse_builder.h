#pragma once

#include "json_utils/json_wrapper.h"

#include "base_utils/variant.h"

#include <string>
#include <vector>

namespace HubPlatform {
namespace CoreD {
namespace Model {

class Value;

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {
namespace GetItems {

using Enum = std::vector< std::string >;

class IResponseBuilder {
public:
    struct OptionalFields {
        std::unique_ptr< CoreD::Model::Value > valueMin;
        std::unique_ptr< CoreD::Model::Value > valueMax;
        std::map< size_t, size_t > elementsMaxNumbers;
        Enum enumeration;
        Enum userCodeModes;
        std::string userCodeRestriction;
        std::string stringRestriction;
        size_t oneWeekDayCost = 0;
        size_t oneShiftedWeekDayCost = 0;
        Json valueIndent;
        Json info;
    };

    virtual ~IResponseBuilder() = default;

    virtual void addItem( const std::string& id,
                          const std::string& deviceId,
                          const std::string& name,
                          bool hasGetter,
                          bool hasSetter,
                          bool show,
                          const CoreD::Model::Value& value,
                          const OptionalFields& optionalFields ) = 0;

    virtual JsonObject buildResponse() const = 0;
};

}  // namespace GetItems

namespace DataListItems {

class IResponseBuilder : public GetItems::IResponseBuilder {
public:
};

}  // namespace DataListItems

}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform
