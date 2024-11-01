#pragma once

#include <functional>

class ReturnAction {
public:
    ReturnAction( std::function< void() > cb )
    : m_cb( cb ) {}
    ~ReturnAction() {
        m_cb();
    }

private:
    std::function< void() > m_cb;
};
