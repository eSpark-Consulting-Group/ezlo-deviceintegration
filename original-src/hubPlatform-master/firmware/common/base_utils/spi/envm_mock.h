#pragma once

#include "base_utils/spi/envm_transport.h"
#include "base_utils/spi/envm_interface.h"

class EnvmMock : public EnvmInterface {
public:
    EnvmMock() = default;
    ~EnvmMock() = default;

public:
    bool setProgrammingOn() override {
        return true;
    }
    bool setProgrammingOff() override {
        return true;
    }
    bool readStatus( EnvmMemoryVector& outResult ) override {
        return true;
    }
    bool readLocks( EnvmMemoryVector& outResult ) override {
        return true;
    }
    bool readID( std::string& outResult ) override {
        return true;
    }
    size_t readData( EnvmMemoryVector& outResult ) override {
        return 0;
    }
    size_t writeData( const EnvmMemoryVector& data ) override {
        return data.size();
    }
    bool bulkErase() override {
        return true;
    }
    size_t size() const override {
        return 0;
    }
};