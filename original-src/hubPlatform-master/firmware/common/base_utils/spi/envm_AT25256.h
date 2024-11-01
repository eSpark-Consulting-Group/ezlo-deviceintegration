#pragma once

#include "base_utils/spi/envm_interface.h"

#include "base_utils/spi/actual_transport.h"
#include "spi/spidev.h"
#include "base_utils/spi/spi_soft_sercomm.h"

class EnvmAt25256 : public EnvmInterface {
public:
    EnvmAt25256();
    ~EnvmAt25256() = default;

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
    static constexpr size_t m_pagesCount = 512;
    static constexpr size_t m_bytesPerPage = 64;
    static constexpr size_t m_size = m_pagesCount * m_bytesPerPage;
    // continous write works with bytes count - 64 only, will read the same count so...
    static constexpr size_t m_bytesReadCount = m_bytesPerPage;
    static constexpr size_t m_bytesWriteCount = m_bytesPerPage;

    const EnvmCommand< EnvmMemoryVector, 5 > m_cmdReadID;
    const EnvmCommand< EnvmMemoryVector, 0 > m_cmdWriteEnable;
    const EnvmCommand< EnvmMemoryVector, 0 > m_cmdWriteDisable;
    const EnvmCommand< EnvmMemoryVector, 1 > m_cmdReadStatus;
    const EnvmCommand< EnvmMemoryVector, 0 > m_cmdWriteStatus;
    const EnvmCommand< EnvmMemoryVector, m_bytesReadCount > m_cmdReadDataBytes;
    const EnvmCommand< EnvmMemoryVector, 0 > m_cmdWriteDataBytes;

private:
    EnvmTransport m_transport;
};