#pragma once

#include "base_utils/runnable.h"

class EventListener : public common::Runnable {
public:
    EventListener();
    virtual ~EventListener() = default;
    virtual void listen() = 0;
    void startInternal() override;
};