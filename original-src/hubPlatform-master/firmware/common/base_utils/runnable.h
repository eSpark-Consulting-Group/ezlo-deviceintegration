#pragma once
#include <atomic>

namespace common {
class Runnable {
public:
    Runnable();
    virtual ~Runnable();

public:
    bool start();
    bool stop();
    bool isRunning() const;

protected:
    virtual void startInternal();
    virtual void stopInternal();

private:
    std::atomic_bool m_isRunning;
};
}  // namespace common
