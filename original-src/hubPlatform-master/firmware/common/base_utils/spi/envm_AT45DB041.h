#pragma once

#include "base_utils/spi/envm_interface.h"

#include "base_utils/spi/actual_transport.h"
#include "base_utils/spi/spi_transport.h"

class EnvmAt45Db041 : public EnvmInterface {
public:
    EnvmAt45Db041();
    ~EnvmAt45Db041() = default;

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
    bool eraseProgrammSucceded();
    bool bulkEraseByErasePages();
    bool bulkEraseByEraseChip();

private:
    static constexpr int32_t m_maxFrequency = 12000000;
    static constexpr size_t m_deviceResetTimeoutUs = 15000;
    static constexpr size_t m_programmingOnTLowUs = 32;
    static constexpr size_t m_programmingOnTHighUs = 127;

    static constexpr size_t m_sectorsCount = 8;
    static constexpr size_t m_pagesCount = 2048;
    static constexpr size_t m_bytesPerPage = 256;
    static constexpr size_t m_size = m_pagesCount * m_bytesPerPage;

    const EnvmCommand< EnvmMemoryVector, 5 > m_cmdReadID;
    const EnvmCommand< EnvmMemoryVector, 2 > m_cmdReadStatus;
    const EnvmCommand< EnvmMemoryVector, 256 > m_cmdReadDataBytes;
    const EnvmCommand< EnvmMemoryVector, 8 > m_cmdReadSectorProtection;
    const EnvmCommand< EnvmMemoryVector, 0 > m_cmdBuffer1Write;
    const EnvmCommand< EnvmMemoryVector, 0 > m_cmdBuffer2Write;
    const EnvmCommand< EnvmMemoryVector, 0 > m_cmdWritePageBuffer1;
    const EnvmCommand< EnvmMemoryVector, 0 > m_cmdWritePageBuffer2;
    const EnvmCommand< EnvmMemoryVector, 0 > m_cmdErasePage;
    const EnvmCommand< EnvmMemoryVector, 0 > m_cmdEraseSector;
    const EnvmCommand< EnvmMemoryVector, 0 > m_cmdEraseChip;
    const EnvmCommand< EnvmMemoryVector, 0 > m_cmdResetChip;

private:
    EnvmTransport m_transport;
};