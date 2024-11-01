#include "cored/model/values/camera_stream_value.h"
#include "cored/model/values/value_visitor.h"
#include "cored/model/exceptions.h"
#include "cored/model/values/value_type.h"
#include "cored/model/values/value_cast.h"
#include "cored/model/resources/cored_resources.h"

#include "json_utils/json_wrapper.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< CameraStreamValue > CameraStreamValue::createValue( const JsonObject& value ) {
    using namespace Resources;
    StreamLink rstpLink, cloudLiveLink;

    auto localIRstpIt = value.find( CameraStreamValueFields::localRtspUrl );
    if( localIRstpIt != value.end() && localIRstpIt->is_string() ) {
        rstpLink = { localIRstpIt->get< std::string >() };
    }

    auto cloudLiveStreamUrlIt = value.find( CameraStreamValueFields::cloudLiveStreamUrl );
    if( cloudLiveStreamUrlIt != value.end() && cloudLiveStreamUrlIt->is_string() ) {
        cloudLiveLink = { cloudLiveStreamUrlIt->get< std::string >() };
    }

    if( !rstpLink.isValid() && !cloudLiveLink.isValid() )
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::CameraStream };

    return std::unique_ptr< CameraStreamValue >(
        new CameraStreamValue( std::move( rstpLink ), std::move( cloudLiveLink ) ) );
}

std::unique_ptr< CameraStreamValue > CameraStreamValue::createDefaultValue() {
    return std::unique_ptr< CameraStreamValue >{ new CameraStreamValue{ {}, {} } };
}

bool CameraStreamValue::isEqual( const Value& other ) const {
    const auto& otherCameraStreamValue = valueCast< CameraStreamValue >( other );
    return !m_rstpLink.isUpdatable( otherCameraStreamValue.m_rstpLink ) &&
           !m_cloudLiveLink.isUpdatable( otherCameraStreamValue.m_cloudLiveLink );
}

void CameraStreamValue::set( const Value& other ) {
    const auto& otherCameraStreamValue = valueCast< CameraStreamValue >( other );

    if( otherCameraStreamValue.m_cloudLiveLink.isValid() ) {
        m_cloudLiveLink = otherCameraStreamValue.m_cloudLiveLink;
    }

    if( otherCameraStreamValue.m_rstpLink.isValid() ) {
        m_rstpLink = otherCameraStreamValue.m_rstpLink;
    }
}

Json CameraStreamValue::toJson() const {
    return JSON( "localRtspUrl" << getLocalRtspUrl() << "cloudLiveStreamUrl" << getCloudLiveStreamUrl() );
}

void CameraStreamValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void CameraStreamValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

std::string CameraStreamValue::getLocalRtspUrl() const {
    return m_rstpLink.isValid() ? m_rstpLink.url() : std::string();
}

std::string CameraStreamValue::getCloudLiveStreamUrl() const {
    return m_cloudLiveLink.isValid() ? m_cloudLiveLink.url() : std::string();
}

CameraStreamValue::CameraStreamValue( StreamLink rstpLink, StreamLink cloudLiveLink )
: Value( ValueType::Enum::CameraStream )
, m_rstpLink( std::move( rstpLink ) )
, m_cloudLiveLink( std::move( cloudLiveLink ) ) {}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
