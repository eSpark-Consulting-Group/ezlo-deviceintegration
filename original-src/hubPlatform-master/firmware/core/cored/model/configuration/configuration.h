#pragma once

#include "base_utils/non_copyable.h"

#include <string>

namespace HubPlatform {
namespace CoreD {
namespace Model {

class ValueType;

class Configuration : private common::NonCopyable {
public:
    virtual ~Configuration() = default;

    virtual std::string getScaleForValueType( const ValueType& ) const = 0;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
