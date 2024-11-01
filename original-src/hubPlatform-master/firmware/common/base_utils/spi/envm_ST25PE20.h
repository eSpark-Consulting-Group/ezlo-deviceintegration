#pragma once

#include "base_utils/spi/envm_interface.h"

#include "base_utils/spi/actual_transport.h"
#include "base_utils/spi/spi_transport.h"

class EnvmSt25Pe20 : public EnvmInterface {
public:
    EnvmSt25Pe20();
    ~EnvmSt25Pe20() = default;

public:
    bool readStatus( EnvmMemoryVector& outResult ) override;
    bool readLocks( EnvmMemoryVector& outResult ) override;
    bool readID( std::string& outResult ) override;
    size_t readData( EnvmMemoryVector& outResult ) override;
    size_t writeData( const EnvmMemoryVector& data ) override;
    bool bulkErase() override;
    size_t size() const override;

protected:
    bool setProgrammingOn() override;
    bool setProgrammingOff() override;

private:
    bool waitChipReady( const int32_t delayUs = 10000, int triesNumber = 10 );
    bool waitChipWriteEnabled( const int32_t delayUs = 10000, int triesNumber = 10 );

private:
    static constexpr int32_t m_maxFrequency = 4000000;
    static constexpr int m_programmingOnTLowUs = 32;
    static constexpr int m_programmingOnTHighUs = 127;

    static constexpr size_t m_sectorsCount = 4;
    static constexpr size_t m_pagesCount = 1024;
    static constexpr size_t m_bytesPerPage = 256;
    static constexpr size_t m_size = m_pagesCount * m_bytesPerPage;
    // very strange but continous read/write works bytes count - 32
    static constexpr size_t m_bytesReadCount = 32;
    static constexpr size_t m_bytesWriteCount = m_bytesReadCount;

    const EnvmCommand< EnvmMemoryVector, 5 > m_cmdReadID;
    const EnvmCommand< EnvmMemoryVector, 1 > m_cmdReadStatus;
    const EnvmCommand< EnvmMemoryVector, 1 > m_cmdReadLocks;
    const EnvmCommand< EnvmMemoryVector, 256 > m_cmdReadDataBytes;
    const EnvmCommand< EnvmMemoryVector, 0 > m_cmdWritePage;
    const EnvmCommand< EnvmMemoryVector, 0 > m_cmdWriteEnable;
    const EnvmCommand< EnvmMemoryVector, 0 > m_cmdEraseSector;
    const EnvmCommand< EnvmMemoryVector, 0 > m_cmdErasePage;

private:
    EnvmTransport m_transport;
};