#include "scenes_manager/scenes_processor/parsers/event_parser_interface.h"
#include "scenes_manager/scenes_processor/parsers/event_parser.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace ScenesManager {

std::unique_ptr< EventParserInterface > EventParserFactory::makeEventParser(
    EventParserInterface::ControllerId controllerId,
    EventParserInterface::GetGeoPositionCallback getGeoPosition,
    EventParserInterface::ClockGetter clockGetter,
    EventParserInterface::MakeGenericValueProvider makeGenericValueProvider,
    Services::TaskLoop& taskLoop ) {
    return std::make_unique< EventParser >( std::move( controllerId ),
                                            std::move( getGeoPosition ),
                                            std::move( clockGetter ),
                                            std::move( makeGenericValueProvider ),
                                            taskLoop );
}

}  // namespace ScenesManager
}  // namespace HubPlatform
