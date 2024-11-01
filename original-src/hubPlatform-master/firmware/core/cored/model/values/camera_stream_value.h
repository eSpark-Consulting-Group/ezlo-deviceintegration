#pragma once
#include "cored/model/values/value.h"
#include <string>

namespace HubPlatform {
namespace CoreD {
namespace Model {

class CameraStreamValue : public Value {
private:
    class StreamLink {
    public:
        StreamLink()
        : m_isValid( false ) {}

        StreamLink( std::string url )
        : m_url( std::move( url ) )
        , m_isValid( true ) {}

        bool isValid() const {
            return m_isValid;
        }
        const std::string& url() const {
            return m_url;
        }

        void operator=( const StreamLink& other ) {
            m_url = other.m_url;
            m_isValid = other.m_isValid;
        }

        bool isUpdatable( const StreamLink& other ) const {
            return other.isValid() && ( !isValid() || url() != other.url() );
        }

    private:
        std::string m_url;
        bool m_isValid;
    };

public:
    static std::unique_ptr< CameraStreamValue > createValue( const JsonObject& value );
    static std::unique_ptr< CameraStreamValue > createDefaultValue();

    Json toJson() const override;
    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

    std::string getLocalRtspUrl() const;
    std::string getCloudLiveStreamUrl() const;

protected:
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    CameraStreamValue( StreamLink rstpLink, StreamLink cloudLiveLink );

private:
    StreamLink m_rstpLink;
    StreamLink m_cloudLiveLink;
};
}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
