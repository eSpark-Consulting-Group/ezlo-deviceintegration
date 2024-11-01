#pragma once

#include <sstream>
#include <chrono>

namespace HubPlatform {
namespace HardwareTest {

class TaskTest;
class TaskQueue;

class TaskVisitor {
public:
    virtual void visit( TaskQueue* task ) const = 0;
    virtual void visit( TaskTest* task ) const = 0;
};

class JoinVisitor : public TaskVisitor {
public:
    virtual void visit( TaskQueue* task ) const override;
    virtual void visit( TaskTest* task ) const override;

private:
    const std::chrono::seconds m_timeout{ 1 };
    const std::chrono::milliseconds m_interval{ 20 };
};

class StatusVisitor : public TaskVisitor {
public:
    virtual void visit( TaskQueue* task ) const override;
    virtual void visit( TaskTest* task ) const override;

public:
    mutable std::stringstream report;
    mutable bool totalResult = true;
};

}  // namespace HardwareTest
}  // namespace HubPlatform
