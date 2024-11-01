#include "cameras/input_output/camera_input.h"
#include "cameras/input_output/camera_input_options.h"
#include "cameras/packet/packet_publisher.h"
#include "cameras/ffmpeg_helpers.h"
#include "base_utils/utils.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
}

namespace HubPlatform {

CameraInput::CameraInput( PacketHandlerSharedPtr packetHandler, std::string url )
: m_packetHandler( std::move( packetHandler ) )
, m_url( std::move( url ) )
, m_maxDelayBetweenPackets( CameraInputOptions::getMaxDelayBetweenPacketsMsec() )
, m_packetsProcessingDelay( CameraInputOptions::getPacketProcessingDelayMsec() )
, m_accumulatedDelayThreshold( CameraInputOptions::getAccumulatedDelayThresholdMsec() )
, m_timeBase( -1 ) {}

CameraInput::~CameraInput() {
    LOGGER_INFO( cc::info( "Releasing camera input object for stream: " ) + cc::note( m_url ) );
    if( m_worker.joinable() ) {
        stop();
        m_worker.join();
    }
    LOGGER_INFO( cc::info( "Closing camera input stream: " ) + cc::note( m_url ) );
    closeStream();
    LOGGER_INFO( cc::info( "Camera input stream closed: " ) + cc::note( m_url ) );
}

bool CameraInput::start( RtspTransport rtspTransport ) {
    if( m_isRunning ) {
        // Reading has been started and is running.
        return true;
    }
    else {
        // Reading has been started and already has finished. Restart.
        if( m_worker.joinable() ) {
            m_worker.join();
            closeStream();
        }

        if( initStreams( rtspTransport ) < 0 ) {
            closeStream();
            return false;
        }
    }

    LOGGER_DEBUG( cc::debug( "Starting camera input thread for stream: " ) + cc::note( m_url ) );
    m_isRunning = true;
    m_stopped = false;
    m_worker = std::thread( [this] {
        LOGGER_INFO_RAW( cc::info( "Camera input thread started for stream: " ) + cc::note( m_url ) +
                         cc::info( ", thread id: " ) + cc::threadId() );
        m_isReceivedKeyFrame = false;

        std::chrono::steady_clock::time_point readFirstTime;
        int64_t firstPts = 0;
        bool lapOnce = true;

        AVPacket avPacket;
        while( !m_stopped ) {
            if( m_isPaused ) {
                std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
                continue;
            }
            if( !m_inputFormatContext ) {
                Log::error( cc::error( "Empty format context" ) );
                m_isRunning = false;
                if( m_listener ) {
                    m_listener->onInputReadingStopped( -1 );
                }
                break;
            }
            int ret = av_read_frame( m_inputFormatContext, &avPacket );
            if( ret < 0 ) {
                Log::warn( cc::warn( "Read frame failed: " ) + cc::attention( FFmpegHelpers::avStringError( ret ) ) );
                m_isRunning = false;
                if( m_listener ) {
                    m_listener->onInputReadingStopped( ret );
                }
                break;
            }

            AVPacketPrototype packetWrapper( avPacket );
            if( ( avPacket.stream_index == m_videoStreamIndex ) && m_isReceivedKeyFrame ) {
                if( lapOnce ) {
                    readFirstTime = std::chrono::steady_clock::now();
                    firstPts = avPacket.pts;
                    lapOnce = false;
                }
                else {
                    using namespace std::chrono;
                    double ptsLap = static_cast< double >( avPacket.pts - firstPts );
                    int64_t ptsLapMs = static_cast< int64_t >( ( ptsLap / m_timeBase ) * 1000.0 );

                    milliseconds timerLap = duration_cast< milliseconds >( steady_clock::now() - readFirstTime );
                    milliseconds totalDelay = timerLap - milliseconds( ptsLapMs );

                    if( totalDelay >= m_accumulatedDelayThreshold ) {
                        Log::warn( cc::warn( "Accumulated delay more than threshold" ) );
                        if( m_listener ) {
                            m_listener->onAccumulatedDelayThresholdReached();
                        }
                        break;
                    }
                }
            }

            if( m_videoStreamIndex == avPacket.stream_index ) {
                auto duration = getDurationBetweenPackets( avPacket );

                // we skip incoming video frames until first key video frame.
                if( !m_isReceivedKeyFrame && avPacket.flags != AV_PKT_FLAG_KEY ) {
                    continue;
                }
                m_isReceivedKeyFrame = true;

                if( duration >= m_maxDelayBetweenPackets ) {
                    if( m_listener ) {
                        m_listener->onMaxPacketsDelayViolated();
                    }

                    auto maxDelayUs =
                        std::chrono::duration_cast< std::chrono::microseconds >( m_maxDelayBetweenPackets );
                    Log::warn( cc::warn( "Packet duration is  " ) + cc::note( std::to_string( duration.count() ) ) +
                               cc::warn( " us more than max " ) + cc::note( std::to_string( maxDelayUs.count() ) ) +
                               cc::warn( " us" ) );
                    m_isReceivedKeyFrame = false;
                    continue;
                }
            }

            m_packetHandler->processPacket( packetWrapper );

            // needed only for unit tests (see CameraInputTestSuite), in general flow this sleep should not be executed
            if( m_packetsProcessingDelay > std::chrono::milliseconds( 0 ) ) {
                std::this_thread::sleep_for( m_packetsProcessingDelay );
            }
        }
        m_isRunning = false;
        Log::debug( cc::debug( "Stream " ) + cc::u( m_url ) + cc::debug( " is stopped" ) );
    } );

    waitForCondition(
        [this] {
            std::this_thread::yield();
            return m_worker.joinable();
        },
        std::chrono::seconds{ 5 },
        std::chrono::milliseconds{ 50 } );

    return true;
}

void CameraInput::pause() {
    m_isPaused = true;
}

void CameraInput::run() {
    m_isPaused = false;
}

void CameraInput::stop() {
    m_stopped = true;
}

bool CameraInput::isRunning() const {
    return m_isRunning;
}

const AVStream* CameraInput::getStream( int avMediaType ) const {
    if( !m_inputFormatContext ) {
        return nullptr;
    }
    for( unsigned i = 0; i < m_inputFormatContext->nb_streams; i++ ) {
        AVStream* stream = m_inputFormatContext->streams[ i ];
        if( stream->codecpar->codec_type == avMediaType )
            return stream;
    }
    return nullptr;
}

const AVStream* CameraInput::getVideoStream() const {
    return getStream( AVMEDIA_TYPE_VIDEO );
}

const AVStream* CameraInput::getAudioStream() const {
    return getStream( AVMEDIA_TYPE_AUDIO );
}

const std::string& CameraInput::getSdp() const {
    return m_sdp;
}

int CameraInput::getVideoIndex() const {
    return m_videoStreamIndex;
}

const AVCodecContext* CameraInput::getInputCodecContext() const {
    return m_codecContext;
}

const PacketHandlerSharedPtr& CameraInput::getPacketHandler() const {
    return m_packetHandler;
}

std::string CameraInput::getError() const {
    return m_error;
}

void CameraInput::setEventsListener( ICameraInputEventsListener* listener ) {
    m_listener = listener;
}

int CameraInput::initStreams( RtspTransport rtspTransport ) {
    int ret;
    FFmpegHelpers::AVDictionaryWrapper opts;
    if( rtspTransport != RtspTransport::Default ) {
        if( ( ret = av_dict_set( &opts, "rtsp_transport", rtspTransport == RtspTransport::Tcp ? "tcp" : "udp", 0 ) ) <
            0 ) {
            m_error = FFmpegHelpers::avStringError( ret );
            Log::error( cc::error( "Could not set option rtsp_transport: " ) + cc::info( m_error ) );
            return ret;
        }
    }

    if( ( ret = av_dict_set( &opts, "max_delay", "1000000", 0 ) ) < 0 ) {  // in microsecond
        m_error = FFmpegHelpers::avStringError( ret );
        Log::error( cc::error( "Could not set option max_delay: " ) + cc::info( m_error ) );
        return ret;
    }

    if( ( ret = av_dict_set( &opts, "stimeout", "5000000", 0 ) ) < 0 ) {  // in microsecond
        m_error = FFmpegHelpers::avStringError( ret );
        Log::error( cc::error( "Could not set option stimeout: " ) + cc::info( m_error ) );
        return ret;
    }

    if( av_dict_set( &opts, "buffer_size", "327680", 0 ) < 0 ) {
        m_error = FFmpegHelpers::avStringError( ret );
        Log::error( cc::error( "Could not set new buffer size" ) );
    }

    if( ( ret = avformat_open_input( &m_inputFormatContext, m_url.c_str(), nullptr, &opts ) ) < 0 ) {
        m_error = FFmpegHelpers::avStringError( ret );
        Log::error( cc::error( "Could not open " ) + cc::u( m_url ) + cc::error( " input: " ) + cc::info( m_error ) );
        return ret;
    }

    if( opts ) {
        AVDictionaryEntry* entry = nullptr;
        std::string message( "Some options were not set: " );
        while( ( entry = av_dict_get( opts, "", entry, AV_DICT_IGNORE_SUFFIX ) ) != nullptr ) {
            message += std::string( entry->key ) + "=" + entry->value + " ";
        }
        Log::warn( message );
    }

    if( ( ret = avformat_find_stream_info( m_inputFormatContext, nullptr ) ) < 0 ) {
        m_error = FFmpegHelpers::avStringError( ret );
        Log::error( cc::error( "Failed to retrieve input stream information: " ) + cc::info( m_error ) );
        return ret;
    }

    AVCodec* codec = nullptr;
    ret = av_find_best_stream( m_inputFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0 );
    if( ret < 0 ) {
        m_error = FFmpegHelpers::avStringError( ret );
        Log::error( cc::error( "Failed to find video stream: " ) + cc::info( m_error ) );
        return ret;
    }
    m_videoStreamIndex = ret;
    AVStream* videoStream = m_inputFormatContext->streams[ m_videoStreamIndex ];

    m_timeBase = videoStream->time_base.den;

    AVCodecParameters* codecParameters = videoStream->codecpar;
    m_codecContext = avcodec_alloc_context3( codec );
    if( m_codecContext == nullptr ) {
        Log::error( cc::error( "Failed to create codec context" ) );
        return -1;
    }

    ret = avcodec_parameters_to_context( m_codecContext, codecParameters );
    if( ret < 0 ) {
        Log::error( cc::error( "Failed to copy decoder parameters to input decoder context" ) );
        return -1;
    }

    m_codecContext->time_base = videoStream->time_base;

    ret = avcodec_open2( m_codecContext, codec, nullptr );
    if( ret < 0 ) {
        m_error = FFmpegHelpers::avStringError( ret );
        Log::error( cc::error( "Could not open codec: " ) + cc::info( m_error ) );
        return ret;
    }

    extractSdp();

    return ret;
}

int CameraInput::readNextPacket( AVPacket& packet ) {
    return av_read_frame( m_inputFormatContext, &packet );
}

void CameraInput::closeStream() {
    avcodec_free_context( &m_codecContext );
    avformat_close_input( &m_inputFormatContext );
}

void CameraInput::extractSdp() {
    // Based on code from https://ffmpeg.org/doxygen/4.1/rtspenc_8c_source.html#l00046

    constexpr int SDP_MAX_SIZE = 16384 /*from ffmpeg*/;
    std::string buf = m_url;
    AVFormatContext sdpCtx, *ctxArray[ 1 ];
    sdpCtx = *m_inputFormatContext;
    sdpCtx.url = &buf[ 0 ];
    ctxArray[ 0 ] = &sdpCtx;
    buf.resize( SDP_MAX_SIZE );
    if( int ret = av_sdp_create( ctxArray, 1, &buf[ 0 ], buf.size() ) ) {
        Log::error( cc::error( "Failed to extract SDP for stream " ) + cc::u( m_url ) + cc::error( " : " ) +
                    cc::info( FFmpegHelpers::avStringError( ret ) ) );
        return;
    }

    // shrink buffer;
    buf.resize( strlen( buf.c_str() ) );

    m_sdp = std::move( buf );

    LOGGER_DEBUG_RAW( cc::debug( "SDP for source: " ) + cc::u( m_url ) + cc::debug( ": " ) + cc::note( "\n" + m_sdp ) );
}

std::chrono::microseconds CameraInput::getDurationBetweenPackets( const AVPacket& packet ) {
    Dts& lastDts = m_lastDts[ packet.stream_index ];
    Dts duration = packet.dts - lastDts;
    lastDts = packet.dts;
    constexpr auto rounding = static_cast< AVRounding >( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX );
    return std::chrono::microseconds(
        av_rescale_q_rnd( duration, m_codecContext->time_base, AV_TIME_BASE_Q, rounding ) );
}

}  // namespace HubPlatform
