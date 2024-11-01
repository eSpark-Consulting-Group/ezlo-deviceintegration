
#include "hardware_manager/speaker_wrapper.h"
#include "hardware_manager/speaker/model/resources.h"

#include "tiny/tiny_consolecolors.h"
#include "log_utils/ezlo_log.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace HardwareManager {

SpeakerWrapper::SpeakerWrapper( Service& service )
#ifdef __HUB_WITH_SPEAKER__
: m_environment( service )
, m_speaker( m_environment )
#endif
{
}

bool SpeakerWrapper::setUp() {
#ifdef __HUB_WITH_SPEAKER__
    return m_speaker.setUp();
#else
    return true;
#endif
}

void SpeakerWrapper::tearDown() {
#ifdef __HUB_WITH_SPEAKER__
    m_speaker.tearDown();
#endif
}

JsonObject SpeakerWrapper::play( const JsonObject& request ) {
    Json answer = JSON_EMPTY_OBJ;

    LOGGER_TRACE( "Speaker request received: " + cc::j( request ) );
#ifdef __HUB_WITH_SPEAKER__
    try {
        std::string songFilePath = json_get_str( request, Speaker::Fields::path );
        auto time = json_get_number< std::int64_t >( request, Speaker::Fields::time );
        auto volume = json_get_number< std::uint8_t >( request, Speaker::Fields::volume );
        std::string plugin = json_get_str( request, Speaker::Fields::plugin );
        std::string handlerScript = json_get_str( request, Speaker::Fields::handler );

        Json userData = JSON_NULL_VAL;
        auto itFind = request.find( Speaker::Fields::userData );
        if( itFind != request.end() ) {
            userData = *itFind;
        }

        auto id = m_speaker.play( std::move( songFilePath ),
                                  time,
                                  volume,
                                  std::move( plugin ),
                                  std::move( handlerScript ),
                                  std::move( userData ) );

        answer[ "result" ] = Json{ { Speaker::Fields::id, id } };
    }
    catch( const std::exception& err ) {
        answer[ "error" ] = std::string( "Error: speaker.play failed: " ) + err.what();
    }
#endif

    LOGGER_TRACE( "Speaker answer: " + cc::j( answer ) );

    return answer;
}

JsonObject SpeakerWrapper::stop( const JsonObject& request ) {
    Json answer = JSON_EMPTY_OBJ;

    LOGGER_TRACE( "Speaker request received: " + cc::j( request ) );
#ifdef __HUB_WITH_SPEAKER__
    try {
        const auto soundId = json_get_str( request, Speaker::Fields::id );
        auto flag = m_speaker.stop( soundId );
        answer[ "result" ] = Json{ { "stopped", flag } };
    }
    catch( const std::exception& err ) {
        answer[ "error" ] = std::string( "Error: speaker.stop failed: " ) + err.what();
    }
#endif
    LOGGER_TRACE( "Speaker answer: " + cc::j( answer ) );

    return answer;
}

}  // namespace HardwareManager
}  // namespace HubPlatform