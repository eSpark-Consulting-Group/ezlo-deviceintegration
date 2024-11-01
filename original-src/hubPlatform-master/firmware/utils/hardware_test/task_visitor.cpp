#include "hardware_test/task_visitor.h"
#include "hardware_test/task_composition.h"
#include "base_utils/utils.h"

#include <iomanip>

namespace HubPlatform {
namespace HardwareTest {

void JoinVisitor::visit( TaskQueue* task ) const {
    auto joinable = [&task]() { return task->m_thread.joinable(); };
    if( waitForCondition( joinable, m_timeout, m_interval ) )
        task->m_thread.join();
}

void JoinVisitor::visit( TaskTest* task ) const {
    auto watchdogJoinable = [&task]() { return task->m_watchdogThread.joinable(); };
    if( waitForCondition( watchdogJoinable, m_timeout, m_interval ) )
        task->m_watchdogThread.join();

    if( task->m_taskStatus != Task::Status::Timeout ) {
        auto joinable = [&task]() { return task->m_thread.joinable(); };
        if( waitForCondition( joinable, m_timeout, m_interval ) )
            task->m_thread.join();
    }
    else
        task->m_thread.detach();
}

void StatusVisitor::visit( TaskQueue* task ) const {}

void StatusVisitor::visit( TaskTest* task ) const {
    bool result = task->m_testResult == TaskTest::TestResult::DONE;
    report << std::left << std::setfill( '.' ) << std::setw( 20 ) << task->m_test.getName();
    report << ( ( result ) ? "PASSED" : "FAILED" );
    if( !result && task->m_taskStatus == Task::Status::Timeout )
        report << " (TIMEOUT)";
    report << std::endl;
    totalResult &= result;
}

}  // namespace HardwareTest
}  // namespace HubPlatform
