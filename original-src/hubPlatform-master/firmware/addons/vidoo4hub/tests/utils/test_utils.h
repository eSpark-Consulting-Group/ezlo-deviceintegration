#pragma once

#include "vidoo4hub/camera/vidoo_camera.h"
#include "vidoo4hub/detectable_types/detectable_types.h"
#include "json_utils/json_fwd.h"
#include <string>
#include <map>

namespace HubPlatform {
namespace UT {

Vidoo4Hub::VidooCamera getDummyCamera( const std::string& cameraId );
std::string getUniqueID();

struct DetectableObject {
    DetectableObject( const std::string& objectId );
    DetectableObject( std::string objectId, std::string objectName );

    std::string id;
    std::string name;
};

struct DetectablePerson {
    DetectablePerson( const std::string& personId );
    DetectablePerson( std::string personId, std::string personName );

    std::string id;
    std::string name;
};

struct DetectableQRCode {
    DetectableQRCode( const std::string& qrCodeId );
    DetectableQRCode( std::string qrCodeId, std::string qrCodeName, std::string qrCodeValue );

    std::string id;
    std::string name;
    std::string value;
};

class MessageBuilder {
public:
    static Json object( std::string objectId, std::string objectName );
    static Json object( const DetectableObject& object );

    static Json person( std::string personId, std::string personName );
    static Json person( const DetectablePerson& person );

    static Json qrCode( std::string qrCodeId, std::string qrCodeName, std::string qrCodeValue );
    static Json qrCode( const DetectableQRCode& qrCode );

    static Json hotzone( const std::string& cameraId, const std::string& hotzoneId, const std::string hotzoneName );

    static std::vector< Json > objectList( const std::vector< DetectableObject >& objects );
    static std::vector< Json > personList( const std::vector< DetectablePerson >& persons );
    static std::vector< Json > qrCodeList( const std::vector< DetectableQRCode >& qrCodes );
};

class DetectableBuilder {
public:
    static Json dictionary( std::string objectId, std::string objectName );
    static Json dictionary( std::string objectId, std::string objectName, std::string objectValue );
    static Json dictionary( const std::string& objectId,
                            const std::string& objectName,
                            const std::string& date,
                            const std::string& itemId,
                            const std::string& hotzoneId );
    static Json dictionary( const std::string& objectId,
                            const std::string& objectName,
                            const std::string& objectValue,
                            const std::string& date,
                            const std::string& itemId,
                            const std::string& hotzoneId );
    static Json dictionary( const DetectableObject& object );
    static Json dictionary( const DetectablePerson& person );
    static Json dictionary( const DetectableQRCode& qrCode );
    static Json dictionary( const std::vector< DetectableObject >& objects );
    static Json dictionary( const std::vector< DetectablePerson >& persons );
    static Json dictionary( const std::vector< DetectableQRCode >& qrCodes );
};

class HotzoneBuilder {
public:
    using ObjectId = std::string;
    using DetectedObjects = std::map< Vidoo4Hub::DetectableTypes::Type, std::set< ObjectId > >;
    static Json hotzone( const std::string& hotzoneName, const DetectedObjects& detectedObjects );
};

}  // namespace UT
}  // namespace HubPlatform
