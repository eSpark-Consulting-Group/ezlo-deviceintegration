#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <memory>
#include <vector>

#include "hardware_test/hardware_tests/hardware_test_base.h"
#include "hardware_test/hardware_tests/hardware_test_interface.h"

namespace HubPlatform {
namespace HardwareTest {

class TaskVisitor;

class TaskCondition {
public:
    void set( bool flag );
    bool get();
    void notify();
    void notify( bool flag );
    void wait();
    void waitFor( const std::chrono::milliseconds& timeout );

private:
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::atomic_bool m_flag{ false };
};

class Task {
public:
    enum class Status { Inactive, Running, Finished, Timeout };

    virtual ~Task() = default;
    virtual void accept( const TaskVisitor& visitor ) = 0;
    virtual void start() = 0;
    virtual void wait() = 0;

protected:
    std::atomic< Task::Status > m_taskStatus{ Task::Status::Inactive };
};

class TaskTest : public Task {
    friend class JoinVisitor;
    friend class StatusVisitor;

public:
    using TestResult = HardwareTestInterface::TestResult;

    TaskTest( HardwareTestBase& test );
    virtual ~TaskTest() = default;
    virtual void accept( const TaskVisitor& visitor ) override;
    virtual void start() override;
    virtual void wait() override;

private:
    void taskThread();

private:
    HardwareTestBase& m_test;
    std::thread m_thread;
    std::thread m_watchdogThread;
    TestResult m_testResult = TestResult::UNKNOWN;
    TaskCondition m_threadFinished;
};

class TaskQueue : public Task {
    friend class JoinVisitor;
    friend class StatusVisitor;

public:
    enum class Order { Serial, Parallel };

    TaskQueue( const Order order );
    virtual ~TaskQueue() = default;
    virtual void accept( const TaskVisitor& visitor ) override;
    virtual void start() override;
    virtual void wait() override;
    void add( std::unique_ptr< Task >&& task );
    size_t size() const;

private:
    void taskThread();

private:
    Order m_order{ Order::Serial };
    std::vector< std::unique_ptr< Task > > m_queue;
    std::thread m_thread;
    TaskCondition m_threadFinished;
};

}  // namespace HardwareTest
}  // namespace HubPlatform
