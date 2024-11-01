#pragma once

#include <string>
#include <array>
#include <stdexcept>

namespace HubPlatform {
namespace VeraProvision {

class ReaderException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

///
/// \brief The ProvisionReaderBase class
/// readAll() must be called first.
class ProvisionReaderBase {
public:
    ProvisionReaderBase() = default;
    ProvisionReaderBase( const ProvisionReaderBase& ) = delete;
    virtual ~ProvisionReaderBase() = default;

    ///
    /// \brief readAll gets devices from /dev/proc and looks for
    /// device with @param devName name. Then reads from it and stores in memory.
    /// May throw ReaderException on any step.
    /// \param devName by default is "factory"
    ///
    virtual void readAll( const std::string& devName = "factory" );

    using MacAddress = std::array< uint8_t, 6 >;
    virtual MacAddress getMacAddress() const;
    virtual std::string getMacAddressString() const;
    virtual std::string getHwKey() const;
    virtual std::string getSerial() const;
    virtual std::string getPassPhrase() const;
    virtual std::string getCountry() const;
    virtual std::string getLanguage() const;
    virtual std::string getWifiSsid() const;

protected:
    ///
    /// \brief The Data class
    /// Internal helper class for extracting values from raw provision data
    ///
    class Data {
    public:
        Data( std::string&& rawData = std::string() );

        const char* getData( size_t offset, size_t length ) const;
        std::string getAsciiString( size_t offset, size_t length ) const;

    private:
        std::string m_rawData;
    };

    ///
    /// \brief The ValueInfo struct
    /// Internal helper struct for storing offset and length of values
    ///
    struct ValueInfo {
        size_t offset;
        size_t length;
    };

    virtual ValueInfo getInfoMacAddress() const = 0;
    virtual ValueInfo getInfoSerialNumber() const = 0;
    virtual ValueInfo getInfoLanguage() const = 0;
    virtual ValueInfo getInfoWifiDomain() const = 0;
    virtual ValueInfo getInfoPassPhrase() const = 0;
    virtual ValueInfo getInfoWifiSsid() const = 0;
    virtual ValueInfo getInfoHwKey() const = 0;

    virtual std::string findMtd( const std::string& mtdName );
    virtual std::string readAllFromDevice( const std::string& fileName, size_t maxLen );

    Data m_data;
};

}  // namespace VeraProvision
}  // namespace HubPlatform

#undef EZLO_LOG_ENABLED
