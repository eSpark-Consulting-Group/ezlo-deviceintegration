#include "scenes_manager/scenes_engine/operators/operator_factory.h"
#include "scenes_manager/scenes_engine/operators/and_operator.h"
#include "scenes_manager/scenes_engine/operators/not_operator.h"
#include "scenes_manager/scenes_engine/operators/or_operator.h"
#include "scenes_manager/scenes_engine/operators/xor_operator.h"
#include "scenes_manager/scenes_engine/operators/for_less_function_operator.h"
#include "scenes_manager/scenes_engine/operators/for_at_least_function_operator.h"
#include "scenes_manager/scenes_engine/operators/repeat_function_operator.h"
#include "scenes_manager/scenes_engine/operators/follow_function_operator.h"
#include "scenes_manager/scenes_engine/operators/pulse_function_operator.h"
#include "service_interfaces/task_loop.h"
#include "scenes_manager/scenes_engine/operators/latch_function_operator.h"
#include "jsonrpc/error_codes.h"
#include "scenes_manager/scenes_engine/exceptions.h"

namespace SceneEngine {

AbstractOperator::SharedOperator OperatorFactory::makeAndOperator() {
    return std::make_shared< AndOperator >();
}

AbstractOperator::SharedOperator OperatorFactory::makeNotOperator() {
    return std::make_shared< NotOperator >();
}

AbstractOperator::SharedOperator OperatorFactory::makeOrOperator() {
    return std::make_shared< OrOperator >();
}

AbstractOperator::SharedOperator OperatorFactory::makeXorOperator() {
    return std::make_shared< XorOperator >();
}

AbstractOperator::SharedOperator OperatorFactory::makeForFunctionOperator( const std::string& method,
                                                                           std::chrono::milliseconds ms,
                                                                           HubPlatform::Services::TaskLoop& taskLoop ) {
    if( method == "less" ) {
        return makeForLessFunctionOperator( ms );
    }
    else if( method == "least" ) {
        return makeForAtLeastFunctionOperator( ms, taskLoop );
    }
    else {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK,
                              std::string{ "Invalid method of FOR function: " }.append( method ).c_str() );
    }
}

AbstractOperator::SharedOperator OperatorFactory::makeForLessFunctionOperator( std::chrono::milliseconds ms ) {
    auto ptr = std::make_shared< ForLessFunctionOperator >();
    ptr->setOptions( ms );
    return ptr;
}

AbstractOperator::SharedOperator OperatorFactory::makeForAtLeastFunctionOperator(
    std::chrono::milliseconds ms,
    HubPlatform::Services::TaskLoop& taskLoop ) {
    auto ptr = std::make_shared< ForAtLeastFunctionOperator >( taskLoop );
    ptr->setOptions( ms );
    return ptr;
}

AbstractOperator::SharedOperator OperatorFactory::makeRepeatFunctionOperator( int times,
                                                                              std::chrono::milliseconds ms ) {
    auto ptr = std::make_shared< RepeatFunctionOperator >();
    ptr->setOptions( times, ms );
    return ptr;
}

SceneEngine::AbstractOperator::SharedOperator OperatorFactory::makeLatchFunctionOperator( bool enabled ) {
    return std::make_shared< SceneEngine::LatchFunctionOperator >( enabled );
}

AbstractOperator::SharedOperator OperatorFactory::makeFollowFunctionOperator( HubPlatform::Services::TaskLoop& taskLoop,
                                                                              std::chrono::seconds resetDelay ) {
    return std::make_shared< FollowFunctionOperator >( taskLoop, resetDelay );
}

AbstractOperator::SharedOperator OperatorFactory::makePulseFunctionOperator( HubPlatform::Services::TaskLoop& taskLoop,
                                                                             std::chrono::seconds truePeriod,
                                                                             std::chrono::seconds falsePeriod,
                                                                             int times ) {
    return std::make_shared< PulseFunctionOperator >( taskLoop, truePeriod, falsePeriod, times );
}

AbstractOperator::SharedOperator OperatorFactory::makePulseFunctionOperator( HubPlatform::Services::TaskLoop& taskLoop,
                                                                             std::chrono::seconds truePeriod,
                                                                             std::chrono::seconds falsePeriod ) {
    return std::make_shared< PulseFunctionOperator >( taskLoop, truePeriod, falsePeriod );
}

}  // namespace SceneEngine
