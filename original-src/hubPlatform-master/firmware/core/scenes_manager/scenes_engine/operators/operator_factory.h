#pragma once
#include "scenes_manager/scenes_engine/operators/scene_operator.h"
#include <chrono>

namespace HubPlatform {
namespace Services {
class TaskLoop;
}
}  // namespace HubPlatform

namespace SceneEngine {

class OperatorFactory final {
public:
    static AbstractOperator::SharedOperator makeAndOperator();
    static AbstractOperator::SharedOperator makeNotOperator();
    static AbstractOperator::SharedOperator makeOrOperator();
    static AbstractOperator::SharedOperator makeXorOperator();

    static AbstractOperator::SharedOperator makeForFunctionOperator( const std::string& method,
                                                                     std::chrono::milliseconds ms,
                                                                     HubPlatform::Services::TaskLoop& taskLoop );
    static AbstractOperator::SharedOperator makeForLessFunctionOperator( std::chrono::milliseconds ms );
    static AbstractOperator::SharedOperator makeForAtLeastFunctionOperator( std::chrono::milliseconds ms,
                                                                            HubPlatform::Services::TaskLoop& taskLoop );
    static AbstractOperator::SharedOperator makeRepeatFunctionOperator( int times, std::chrono::milliseconds ms );
    static AbstractOperator::SharedOperator makeFollowFunctionOperator( HubPlatform::Services::TaskLoop& taskLoop,
                                                                        std::chrono::seconds resetDelay );

    static AbstractOperator::SharedOperator makePulseFunctionOperator( HubPlatform::Services::TaskLoop& taskLoop,
                                                                       std::chrono::seconds truePeriod,
                                                                       std::chrono::seconds falsePeriod,
                                                                       int times );
    static AbstractOperator::SharedOperator makePulseFunctionOperator( HubPlatform::Services::TaskLoop& taskLoop,
                                                                       std::chrono::seconds truePeriod,
                                                                       std::chrono::seconds falsePeriod );
    static AbstractOperator::SharedOperator makeLatchFunctionOperator( bool enabled );
};

}  // namespace SceneEngine
