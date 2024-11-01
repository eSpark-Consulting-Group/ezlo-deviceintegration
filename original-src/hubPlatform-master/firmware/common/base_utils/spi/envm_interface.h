#pragma once

#include "base_utils/spi/envm_transport.h"

namespace envm {

std::string hexToString( const unsigned char* data, int size, bool upperCase = true );

}  // namespace envm

template < typename ContainerT, int resultSize = -1 >
class EnvmCommand {
public:
    EnvmCommand() = default;
    EnvmCommand( const ContainerT& source )
    : m_command( source ){};
    ~EnvmCommand() = default;

public:
    int getCommandSize() const {
        return m_command.size();
    }
    int getResultSize() const {
        return m_resultSize;
    }
    const ContainerT& getCommand() const {
        return m_command;
    };

private:
    const int m_resultSize = resultSize;
    const ContainerT m_command;
};

class EnvmInterface {
public:
    using EnvmMemoryVector = std::vector< uint8_t >;

public:
    EnvmInterface() = default;
    virtual ~EnvmInterface() = default;

public:
    virtual bool readStatus( EnvmMemoryVector& outResult ) = 0;
    virtual bool readLocks( EnvmMemoryVector& outResult ) = 0;
    virtual bool readID( std::string& outResult ) = 0;
    virtual size_t readData( EnvmMemoryVector& outResult ) = 0;
    virtual size_t writeData( const EnvmMemoryVector& data ) = 0;
    virtual bool bulkErase() = 0;
    virtual size_t size() const = 0;

protected:
    virtual bool setProgrammingOn() = 0;
    virtual bool setProgrammingOff() = 0;

    /*
    protected:
        void registerTransport( EnvmTransportInterface& transport ) {
            m_transport = transport;
        }

        int32_t init( uint32_t speed, const bool keepCSActivated = true ) {
            return m_transport.init( speed, keepCSActivated );
        }

        void deinit() {
            m_transport.deinit();
        }

        int getDescriptor() const {
            return m_transport.getDescriptor();
        }

        int sendRequest( const std::vector< uint8_t >& command, int32_t resultSize, std::vector< uint8_t >& outResult )
    { return m_transport.sendRequest( command, resultSize, outResult );
        }

        bool isValid() const {
            return m_transport.isValid();
        }

    private:
        EnvmTransportInterface& m_transport;
    */
};
