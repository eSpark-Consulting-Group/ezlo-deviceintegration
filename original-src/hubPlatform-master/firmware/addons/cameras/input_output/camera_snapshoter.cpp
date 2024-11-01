#include "cameras/input_output/camera_snapshoter.h"
#include "cameras/input_output/camera_snapshoter_options.h"
#include "cameras/codec/codec_context.h"
#include "cameras/frame/frame.h"
#include "cameras/packet/packet_publisher.h"
#include "cameras/storages/storage.h"
#include "cameras/ffmpeg_helpers.h"
#include "log_utils/ezlo_log.h"
#include "base_utils/gregorian_calendar.h"
#include "base_utils/utils.h"
#include "json_utils/json_utils.h"
#include "system_utils/environment.h"
#include "base_utils/timeutils.h"
#include <string>
#include <utility>

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
}

namespace HubPlatform {

class CameraSnapshoterException : public std::exception {
public:
    explicit CameraSnapshoterException( std::string message )
    : m_message( std::move( message ) ) {}
    const char* what() const noexcept override {
        return m_message.c_str();
    }

private:
    std::string m_message;
};

const size_t CameraSnapshoter::NumberOfThreads;

CameraSnapshoter::CameraSnapshoter( std::string snapshoterId )
: m_packetBuffer( CameraSnapshoterOptions::getBufferSize(), PacketsBuffer::SkippingType::Keep )
, m_isRunning( false )
, m_threadPool( NumberOfThreads, CameraSnapshoterOptions::getQueueLimit() )
, m_timeout( CameraSnapshoterOptions::getTimeoutSec() )
, m_snapshoterId( std::move( snapshoterId ) ) {}

void CameraSnapshoter::init( const ICameraInput& cameraInput ) {
    int ret = -1;
    const AVStream* stream = cameraInput.getVideoStream();
    if( stream == nullptr ) {
        throw CameraSnapshoterException( "No video stream" );
    }
    const AVCodecContext* inputCodecCtx = cameraInput.getInputCodecContext();
    if( inputCodecCtx == nullptr ) {
        throw CameraSnapshoterException( "No video codec context" );
    }

    const AVStream* videoStream = cameraInput.getVideoStream();
    if( videoStream == nullptr ) {
        throw CameraSnapshoterException( "No video stream" );
    }

    m_videoStreamIndex = cameraInput.getVideoIndex();

    // Find the decoder for the video stream
    AVCodec* codec = avcodec_find_decoder( inputCodecCtx->codec_id );

    if( codec == nullptr ) {
        throw CameraSnapshoterException( "Codec not found" );
    }

    m_inputCodecContext = AVCodecContextWrapper( codec );
    if( m_inputCodecContext.getContext() == nullptr ) {
        throw CameraSnapshoterException( "Could not allocate video codec context" );
    }

    ret = avcodec_parameters_to_context( m_inputCodecContext.getContext(), videoStream->codecpar );
    if( ret < 0 ) {
        throw CameraSnapshoterException( "Failed to copy decoder parameters to input decoder context: " +
                                         FFmpegHelpers::avStringError( ret ) );
    }

    ret = avcodec_open2( m_inputCodecContext.getContext(), codec, nullptr );
    if( ret < 0 ) {
        throw CameraSnapshoterException( "Could not open codec: " + FFmpegHelpers::avStringError( ret ) );
    }

    m_outputCodec = avcodec_find_encoder( AV_CODEC_ID_MJPEG );
    if( !m_outputCodec ) {
        throw CameraSnapshoterException( "Codec not found" );
    }

    m_packetHandler = cameraInput.getPacketHandler();
    m_inputStream.pixelFormat = inputCodecCtx->pix_fmt;
    m_inputStream.resolution.width = inputCodecCtx->width;
    m_inputStream.resolution.height = inputCodecCtx->height;
    m_inputStream.bit_rate = inputCodecCtx->bit_rate;
    m_inputStream.time_base = inputCodecCtx->time_base;
    m_isInitialized = true;
}

void CameraSnapshoter::removeExpiredSnapshots( std::chrono::seconds expirationPeriod ) {
    auto savedImagesNames = filesystem::getDirectoryFileNames( Environment::getSnapshotsFolder().c_str() );
    if( savedImagesNames.empty() )
        return;
    for( auto savedImagesFileName : savedImagesNames ) {
        if( savedImagesFileName.find( m_snapshoterId ) != std::string::npos ) {
            auto imagePath = Environment::getSnapshotsFolder() + "/" + savedImagesFileName;
            struct stat attr;
            if( stat( imagePath.c_str(), &attr ) != 0 ) {
                LOGGER_DEBUG( "Could not get file attributes: " + imagePath );
            }
            auto currentTime = std::chrono::system_clock::now();
            auto fileModificationTime = std::chrono::system_clock::from_time_t( attr.st_mtime );
            auto fileAge = std::chrono::duration_cast< std::chrono::seconds >( currentTime - fileModificationTime );
            if( fileAge > expirationPeriod ) {
                LOGGER_DEBUG( "Remove expired snapshot image: " + imagePath );
                if( !filesystem::removeFile( imagePath ) ) {
                    LOGGER_DEBUG( "Can't remove file: " + imagePath );
                }
            }
        }
    }
}

bool CameraSnapshoter::makeSnapshot( StorageSharedPtr storage,
                                     const ResultCallback& resultCallback,
                                     const std::string& requestId,
                                     const std::shared_ptr< Resolution >& resolution ) {
    LOGGER_DEBUG( "makeSnapshot requestId: " + requestId );

    const std::chrono::seconds SnapshotsExpiredPeriod( 60 );
    removeExpiredSnapshots( SnapshotsExpiredPeriod );

    if( !storage ) {
        const auto error = "empty storage";
        LOGGER_ERROR( error );
        resultCallback( createError( error ) );
        return false;
    }
    if( !isInitialized() ) {
        const auto error = "not initialized";
        LOGGER_ERROR( error );
        resultCallback( createError( error ) );
        return false;
    }

    LOGGER_DEBUG( "create AVCodecContextWrapper requestId: " + requestId );
    auto codecContextPtr = std::make_shared< AVCodecContextWrapper >();
    AVCodecContext* avCodecContext = codecContextPtr->getContext();
    if( !avCodecContext ) {
        const auto error = "Could not allocate codec";
        LOGGER_ERROR( error );
        resultCallback( createError( error ) );
        return false;
    }

    avCodecContext->bit_rate = m_inputStream.bit_rate;
    avCodecContext->pix_fmt = AV_PIX_FMT_YUVJ420P;
    avCodecContext->color_range = AVCOL_RANGE_JPEG;
    avCodecContext->codec_id = AV_CODEC_ID_MJPEG;
    avCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
    avCodecContext->time_base = m_inputStream.time_base;

    LOGGER_DEBUG( "setup avCodecContext requestId: " + requestId );

    if( resolution ) {
        if( resolution->height == 0 ) {
            auto proportion = static_cast< float >( m_inputStream.resolution.height ) / m_inputStream.resolution.width;
            avCodecContext->width = resolution->width;
            avCodecContext->height = proportion * resolution->width;
        }
        else if( resolution->width == 0 ) {
            auto propotion = static_cast< float >( m_inputStream.resolution.width ) / m_inputStream.resolution.height;
            avCodecContext->width = propotion * resolution->height;
            avCodecContext->height = resolution->height;
        }
        else {
            avCodecContext->width = resolution->width;
            avCodecContext->height = resolution->height;
        }
    }
    else {
        avCodecContext->width = m_inputStream.resolution.width;
        avCodecContext->height = m_inputStream.resolution.height;
    }

    LOGGER_DEBUG( "open avcodec requestId: " + requestId );
    int ret = avcodec_open2( avCodecContext, m_outputCodec, nullptr );
    if( ret < 0 ) {
        const auto error = "Could not allocate codec" + FFmpegHelpers::avStringError( ret );
        LOGGER_ERROR( error );
        resultCallback( createError( error ) );
        return false;
    }
    LOGGER_DEBUG( "open avcodec - done requestId: " + requestId );
    auto snapshotNumber = m_snapshotCounter++;
    try {
        m_threadPool.submit_without_future( [this, snapshotNumber, resultCallback, codecContextPtr, storage]() {
            bool isKeyFrameFound = false;
            PacketListenerRemover packetListenerRemover(
                *m_packetHandler,
                [this, isKeyFrameFound]( const AVPacketPrototype& packet ) mutable {
                    if( !isKeyFrameFound ) {
                        isKeyFrameFound = ( packet.getPacket().flags & AV_PKT_FLAG_KEY ) &&
                                          ( packet.getPacket().pts != AV_NOPTS_VALUE );
                        if( !isKeyFrameFound ) {
                            return;
                        }
                    }
                    receivePacket( packet );
                },
                m_videoStreamIndex );
            packetListenerRemover.setActionAfterListenerRemoving( [this]() { m_packetBuffer.clear(); } );
            auto date = GregorianCalendar::Date( std::chrono::system_clock::now() ).toString();
            auto fileSender = storage->createFileSender( m_snapshoterId + "_" + date + "_" +
                                                         std::to_string( snapshotNumber ) + ".jpg" );

            if( !fileSender ) {
                return;
            }
            fileSender->setResultCallback( resultCallback );
            waitAndProcessPacket( *fileSender, codecContextPtr->getContext(), resultCallback );
        } );
    }
    catch( std::exception& ex ) {
        LOGGER_ERROR( ex.what() + ", requestId: " + requestId );
        resultCallback( createError( ex.what() ) );
        return false;
    }
    return true;
}

bool CameraSnapshoter::isInitialized() const {
    return m_isInitialized;
}

void CameraSnapshoter::receivePacket( const AVPacketPrototype& packet ) {
    m_packetBuffer.appendElement( packet );
}

bool CameraSnapshoter::waitAndProcessPacket( FileSender& fileSender,
                                             AVCodecContext* avCodecContext,
                                             const ResultCallback& resultCallback ) {
    m_isRunning = true;
    auto startTimepoint = std::chrono::steady_clock::now();
    while( m_isRunning ) {
        AVPacketPrototype packet;
        if( m_packetBuffer.getFrontElement( packet ) ) {
            DecodeResult result = decodeVideo( m_inputCodecContext.getContext(), packet.getPacket() );
            if( result.code < 0 ) {
                LOGGER_WARN( "video decoding failed: " + FFmpegHelpers::avStringError( result.code ) );
                resultCallback( createError( "video decoding failed" ) );
                return false;
            }
            if( result.gotResult ) {
                return processFrame( fileSender, avCodecContext, result.frame.getFrame(), resultCallback );
            }
        }
        else {
            // Wait for next portion of frames.
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
            auto now = std::chrono::steady_clock::now();
            auto time = now - startTimepoint;
            if( time > m_timeout ) {
                std::string error = "snapshot timeout";
                LOGGER_WARN( error );
                resultCallback( createError( error ) );
                return false;
            }
        }
    }
    return false;
}

bool CameraSnapshoter::processFrame( FileSender& fileSender,
                                     AVCodecContext* avCodecContext,
                                     AVFrame* avInputFrame,
                                     const ResultCallback& resultCallback ) {
    avCodecContext->qmin = avCodecContext->qmax = 3;
    avCodecContext->mb_lmin = avCodecContext->qmin * FF_QP2LAMBDA;
    avCodecContext->mb_lmax = avCodecContext->qmax * FF_QP2LAMBDA;
    avCodecContext->flags |= AV_CODEC_FLAG_QSCALE;

    avInputFrame->quality = 4;
    avInputFrame->pts = m_snapshotCounter;

    AVFrame* avOutputFrame = nullptr;
    AVFrameWrapper outputFrame;
    std::shared_ptr< uint8_t > image;
    if( m_inputStream.resolution != Resolution{ avCodecContext->width, avCodecContext->height } ) {
        avOutputFrame = outputFrame.getFrame();
        memset( avOutputFrame, 0, sizeof( AVFrame ) );
        avOutputFrame->width = avCodecContext->width;
        avOutputFrame->height = avCodecContext->height;

        int ret = av_image_alloc( avOutputFrame->data,
                                  avOutputFrame->linesize,
                                  avOutputFrame->width,
                                  avOutputFrame->height,
                                  avCodecContext->pix_fmt,
                                  32 );
        if( ret < 0 ) {
            std::string error = "image allocation error: " + FFmpegHelpers::avStringError( ret );
            LOGGER_ERROR( error );
            resultCallback( createError( error ) );
            return false;
        }
        image = std::shared_ptr< uint8_t >( reinterpret_cast< uint8_t* >( avOutputFrame->data ), av_freep );

        std::unique_ptr< SwsContext, decltype( &sws_freeContext ) > swsContext(
            sws_getCachedContext( NULL,
                                  avInputFrame->width,
                                  avInputFrame->height,
                                  m_inputStream.pixelFormat,
                                  avOutputFrame->width,
                                  avOutputFrame->height,
                                  avCodecContext->pix_fmt,
                                  SWS_BICUBIC,
                                  NULL,
                                  NULL,
                                  NULL ),
            sws_freeContext );
        if( !swsContext ) {
            std::string error = "image allocation error: " + FFmpegHelpers::avStringError( ret );
            LOGGER_ERROR( error );
            resultCallback( createError( error ) );
            return false;
        }
        ret = sws_scale( swsContext.get(),
                         (const uint8_t* const*) avInputFrame->data,
                         avInputFrame->linesize,
                         0,
                         avInputFrame->height,
                         avOutputFrame->data,
                         avOutputFrame->linesize );
        if( ret < 0 ) {
            std::string error = "image scale error: " + FFmpegHelpers::avStringError( ret );
            LOGGER_ERROR( error );
            resultCallback( createError( error ) );
            return false;
        }
    }
    else {
        avOutputFrame = avInputFrame;
    }
    auto result = encodeVideo( avCodecContext, *avOutputFrame );
    if( result.code < 0 || !result.gotResult ) {
        fileSender.setFileValidity( false );
        std::string error = "encode video error: " + FFmpegHelpers::avStringError( result.code );
        LOGGER_ERROR( error );
        resultCallback( createError( error ) );
        return false;
    }
    AVPacket* avOutputPacket = result.packet.getPacket();
    fileSender.sendData( avOutputPacket->data, avOutputPacket->size );
    fileSender.finalizeFile();
    return true;
}

Json CameraSnapshoter::createError( const std::string& errorMessage ) const {
    return JSON( "error" << JSON( "message" << errorMessage ) );
}

CameraSnapshoter::DecodeResult CameraSnapshoter::decodeVideo( AVCodecContext* codecContext, const AVPacket& packet ) {
    DecodeResult result;
    result.code = avcodec_send_packet( codecContext, &packet );
    if( result.code < 0 ) {
        return result;
    }
    result.code = avcodec_receive_frame( codecContext, result.frame.getFrame() );
    if( result.code == 0 ) {
        result.gotResult = true;
    }
    else if( result.code == AVERROR( EAGAIN ) ) {
        result.code = 0;
    }
    return result;
}

CameraSnapshoter::EncodeResult CameraSnapshoter::encodeVideo( AVCodecContext* codecContext, const AVFrame& frame ) {
    EncodeResult result;
    result.code = avcodec_send_frame( codecContext, &frame );

    bool flushed = false;
    while( result.code >= 0 ) {
        AVPacketWrapper packet;
        result.code = avcodec_receive_packet( codecContext, packet.getPacket() );

        if( result.code == 0 ) {
            result.gotResult = true;
        }
        else if( result.code == AVERROR( EAGAIN ) ) {
            result.code = 0;
        }

        if( result.code < 0 ) {
            return result;
        }

        if( flushed ) {
            return result;
        }

        // flush
        result.code = avcodec_send_frame( codecContext, nullptr );
        flushed = true;

        if( result.gotResult ) {
            result.packet = std::move( packet );
            break;
        }
    }
    return result;
}

}  // namespace HubPlatform
