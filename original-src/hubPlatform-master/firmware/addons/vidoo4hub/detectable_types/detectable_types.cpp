#include "vidoo4hub/detectable_types/detectable_types.h"

#include <map>

#include "cored/model/resources/cored_resources.h"

namespace HubPlatform {
namespace Vidoo4Hub {
namespace DetectableTypes {

const std::set< Type >& getKnownTypes() {
    static const std::set< Type > types = { Type::Object, Type::Person, Type::QRCode };
    return types;
}

const std::string& getHotzoneTypeName( Type type ) {
    namespace HotzoneFields = HubPlatform::CoreD::Model::Resources::CameraHotzoneValueFields;
    static const std::map< Type, std::string > names = { { Type::Object, HotzoneFields::Objects },
                                                         { Type::Person, HotzoneFields::Persons },
                                                         { Type::QRCode, HotzoneFields::QRCodes } };
    return names.at( type );
}

}  // namespace DetectableTypes
}  // namespace Vidoo4Hub
}  // namespace HubPlatform
