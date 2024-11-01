#include "hardware_test/task_composition.h"
#include "hardware_test/task_visitor.h"

namespace HubPlatform {
namespace HardwareTest {

void TaskCondition::set( bool flag ) {
    std::lock_guard< std::mutex > lock( m_mutex );
    m_flag = flag;
}

bool TaskCondition::get() {
    return m_flag.load();
}

void TaskCondition::notify() {
    m_condition.notify_all();
}

void TaskCondition::notify( bool flag ) {
    set( flag );
    notify();
}

void TaskCondition::wait() {
    std::unique_lock< std::mutex > lock( m_mutex );
    m_condition.wait( lock, [this] { return m_flag.load(); } );
}

void TaskCondition::waitFor( const std::chrono::milliseconds& timeout ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    m_condition.wait_for( lock, timeout, [this] { return m_flag.load(); } );
}

TaskTest::TaskTest( HardwareTestBase& test )
: m_test( test ) {}

void TaskTest::accept( const TaskVisitor& visitor ) {
    visitor.visit( this );
}

void TaskTest::start() {
    m_taskStatus = Task::Status::Running;
    m_thread = std::thread( &TaskTest::taskThread, this );

    auto watchdog = [this]() {
        m_threadFinished.waitFor( m_test.getTimeout() );
        if( m_taskStatus != Task::Status::Finished )
            m_taskStatus = Task::Status::Timeout;
        m_threadFinished.notify( true );
    };
    m_watchdogThread = std::thread( watchdog );
}

void TaskTest::wait() {
    m_threadFinished.wait();
}

void TaskTest::taskThread() {
    m_taskStatus = Task::Status::Running;
    m_testResult = m_test.run();
    m_taskStatus = Task::Status::Finished;
    m_threadFinished.notify( true );
}

TaskQueue::TaskQueue( const Order order )
: m_order( order ) {}

void TaskQueue::accept( const TaskVisitor& visitor ) {
    for( auto& task : m_queue )
        task->accept( visitor );
    visitor.visit( this );
}

void TaskQueue::start() {
    m_taskStatus = Task::Status::Running;
    m_thread = std::thread( &TaskQueue::taskThread, this );
}

void TaskQueue::wait() {
    m_threadFinished.wait();
}

void TaskQueue::add( std::unique_ptr< Task >&& task ) {
    m_queue.push_back( std::move( task ) );
}

size_t TaskQueue::size() const {
    return m_queue.size();
}

void TaskQueue::taskThread() {
    m_taskStatus = Task::Status::Running;
    for( auto& task : m_queue ) {
        task->start();
        if( m_order == Order::Serial )
            task->wait();
    }

    for( auto& task : m_queue )
        task->wait();

    m_taskStatus = Task::Status::Finished;
    m_threadFinished.notify( true );
}

}  // namespace HardwareTest
}  // namespace HubPlatform
