#include "cameras/input_output/camera_output.h"
#include "cameras/input_output/camera_output_options.h"
#include "cameras/input_output/video_converter.h"
#include "cameras/input_output/audio_converter.h"
#include "cameras/input_output/resolution.h"
#include "cameras/storages/storage.h"
#include "cameras/storages/file_sender.h"
#include "cameras/codec/codec_context.h"
#include "cameras/ffmpeg_helpers.h"

#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"

#include "base_utils/gregorian_calendar.h"

#include <tiny/tiny_consolecolors.h>

extern "C" {
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
}

#include <cassert>

namespace HubPlatform {

namespace {
constexpr size_t outputBufferSize = 4 * 1024;  // Typical size equals pagesize, 4KB
}  // namespace

CameraOutput::CameraOutput( StorageSharedPtr storage,
                            std::string fileNameStart,
                            std::chrono::seconds prerecordedDuration,
                            std::chrono::seconds segmentDuration,
                            int segmentsLimit,
                            const std::string& /*format*/ )
: m_shouldStart( false )
, m_shouldStop( false )
, m_shouldCancel( false )
, m_stopped( true )
, m_buffer( CameraOutputOptions::getRecordBufferSize() )
, m_storage( std::move( storage ) )
, m_fileNameStart( std::move( fileNameStart ) )
, m_memBuffer( static_cast< decltype( m_memBuffer ) >( av_malloc( outputBufferSize ) ) )
, m_segmentDurationMcs( AV_TIME_BASE * segmentDuration.count() )
, m_prerecordedDurationMcs( AV_TIME_BASE * prerecordedDuration.count() )
, m_segmentsLimit( segmentsLimit )
, m_fileExtension( "mp4" )
, m_outputMuxer( "mp4" )
, m_prerecordBufferSize( CameraOutputOptions::getPreRecordBufferSize() ) {
    assert( m_memBuffer );
    // since MULAW audio format is converted to AAC audio format we can write record to mp4 container
    // the mov container is not needed anymore
    //    if( format == "mov" ) {
    //        m_fileExtension = "mov";
    //        m_outputMuxer = "mov";
    //    }
}

CameraOutput::~CameraOutput() {
    LOGGER_DEBUG_RAW( cc::debug( "CameraOutput: Releasing camera output ..." ) );
    cancel();
    if( m_memBuffer )
        av_freep( &m_memBuffer );
    m_memBuffer = nullptr;
}

int CameraOutput::getPrerecordedDuration() const {
    return m_prerecordedDurationMcs / AV_TIME_BASE;
}

int CameraOutput::getSegmentDuration() const {
    return m_segmentDurationMcs / AV_TIME_BASE;
}

int CameraOutput::getSegmentsLimit() const {
    return m_segmentsLimit;
}

int CameraOutput::processPacket( AVPacketPrototype& packet ) {
    int ret = 0;
    AVPacket& pkt = packet.getPacket();

    // Packets from stream that we don't want to write may be already have been added to the buffer.
    if( !checkStreamIndex( pkt.stream_index ) ) {
        return ret;
    }
    AVStream* outStream = m_currentSegment->outputFormatContext->streams[ pkt.stream_index ];

    const AVRational& inputTimeBase = m_streamParams[ pkt.stream_index ].inputTimeBase;
    constexpr auto rounding = static_cast< AVRounding >( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX );
    pkt.pts = av_rescale_q_rnd( pkt.pts, inputTimeBase, outStream->time_base, rounding );
    pkt.dts = av_rescale_q_rnd( pkt.dts, inputTimeBase, outStream->time_base, rounding );

    int64_t& lastDts = m_lastDts.emplace( pkt.stream_index, pkt.dts ).first->second;
    if( pkt.dts != AV_NOPTS_VALUE && lastDts != AV_NOPTS_VALUE && pkt.dts <= lastDts ) {
        fixPacketTimestamps( pkt, lastDts );
    }
    pkt.duration = pkt.dts - lastDts;
    lastDts = pkt.dts;

    if( m_shouldStart ) {
        m_stopped = false;
        m_shouldStart = false;
        m_offsets.clear();
    }

    if( m_stopped ) {
        m_shouldStop = false;
        // Remove all non key-frames at front and key frames that are older then prerecordedDuration
        if( m_currentSegment->referenceStream == pkt.stream_index ) {
            while( !m_prerecorded.empty() ) {
                auto& frontPacket = m_prerecorded.front().getPacket();
                if( frontPacket.pts > pkt.pts && pkt.pts == AV_NOPTS_VALUE ) {
                    m_prerecorded.clear();
                    break;
                }
                if( ( frontPacket.stream_index == pkt.stream_index ) && ( frontPacket.flags & AV_PKT_FLAG_KEY ) &&
                    ( frontPacket.pts != AV_NOPTS_VALUE ) &&
                    ( av_compare_ts(
                          pkt.pts - frontPacket.pts, outStream->time_base, m_prerecordedDurationMcs, AV_TIME_BASE_Q ) <=
                      0 ) ) {
                    break;
                }
                m_prerecorded.pop_front();
            }
        }
        if( m_prerecorded.size() > m_prerecordBufferSize ) {
            Log::error( cc::error( "Max buffer size was reached: " ) + cc::num( m_prerecorded.size() ) +
                        cc::error( ", buffer clearing" ) );
            m_prerecorded.clear();
        }
        m_prerecorded.push_back( std::move( packet ) );
        return ret;
    }

    while( !m_prerecorded.empty() ) {
        postProcessPacket( m_prerecorded.front() );
        m_prerecorded.pop_front();
    }

    return postProcessPacket( packet );
}

int CameraOutput::postProcessPacket( AVPacketPrototype& packet ) {
    int ret = 0;

    if( m_stopped ) {
        return ret;
    }

    AVPacket& pkt = packet.getPacket();
    AVStream* outStream = m_currentSegment->outputFormatContext->streams[ pkt.stream_index ];
    bool isReferenceStream = m_currentSegment->referenceStream == pkt.stream_index;

    auto offsetIt = m_offsets.find( pkt.stream_index );
    if( offsetIt != m_offsets.end() ) {
        pkt.pts -= offsetIt->second;
        pkt.dts -= offsetIt->second;
    }
    else {
        LOGGER_TRACE( "pts: " + std::to_string( pkt.pts ) + ", dts: " + std::to_string( pkt.dts ) );
        int64_t pts = pkt.pts > 0 ? pkt.pts : 0;
        int64_t dts = pkt.dts > 0 ? pkt.dts : 0;
        auto offset = pts < dts ? pts : dts;
        LOGGER_TRACE( "offset: " + std::to_string( offset ) );
        m_offsets[ pkt.stream_index ] = offset;
        pkt.pts -= offset;
        pkt.dts -= offset;
    }

    if( m_desiredDurationMcs > 0 &&
        av_compare_ts( pkt.pts, outStream->time_base, m_desiredDurationMcs, AV_TIME_BASE_Q ) >= 0 ) {
        // finish recording
        Log::debug( cc::debug( "Reached max recording duration" ) );
        m_shouldStop = true;
        m_recordNumber++;
    }

    if( m_shouldStop ) {
        ret = segmentEnd();
        if( ret < 0 ) {
            return ret;
        }
        m_stopped = true;
        m_shouldStop = false;
        ret = segmentStart( true );
        return ret;
    }

    // Additional check before to do something heavy with packet.
    if( m_shouldCancel ) {
        return AVERROR_EOF;
    }

    if( isReferenceStream && ( pkt.flags & AV_PKT_FLAG_KEY ) && ( m_currentSegment->segmentPacketCount > 0 ) &&
        ( pkt.pts != AV_NOPTS_VALUE && av_compare_ts( pkt.pts,
                                                      outStream->time_base,
                                                      /*endPts*/ m_segmentDurationMcs * ( m_segmentCount + 1 ),
                                                      AV_TIME_BASE_Q ) >= 0 ) ) {
        Log::debug( cc::debug( "Start new segment" ) );
        // End current segment
        ret = segmentEnd();
        if( ret < 0 ) {
            return ret;
        }

        if( m_segmentsLimit != 0 && m_segmentCount >= m_segmentsLimit ) {
            Log::debug( cc::debug( "Reached segments limit" ) );
            m_stopped = true;
            m_shouldStop = false;
            m_recordNumber++;
            ret = segmentStart( true );
            return ret;
        }

        // Start new segment
        ret = segmentStart( false );
        if( ret < 0 ) {
            return ret;
        }
    }

    ret = m_currentSegment->write( pkt );
    if( ret < 0 ) {
        Log::error( cc::error( "Error muxing packet: " + FFmpegHelpers::avStringError( ret ) ) );
    }
    else {
        if( isReferenceStream ) {
            ++m_currentSegment->segmentPacketCount;
        }
    }
    return ret;
}

void CameraOutput::start( std::chrono::seconds maxDurationSec ) {
    m_desiredDurationMcs = maxDurationSec.count() * AV_TIME_BASE;
    m_shouldStart = true;
}

bool CameraOutput::isRecording() const noexcept {
    return !m_stopped;
}

int CameraOutput::startSegment() {
    int ret = segmentStart( true );
    if( ret == AVERROR( EINVAL ) ) {
        // We cannot define a reason of the error, but perhaps it's because of
        // unsupported audio codec, try to record without audio stream.
        auto it =
            std::find_if( m_streamParams.begin(), m_streamParams.end(), []( const TStreamParams::value_type& params ) {
                return params.second.codecParams.get()->codec_type == AVMEDIA_TYPE_AUDIO;
            } );
        if( it != m_streamParams.end() ) {
            Log::info( cc::info( "Trying to record without audio stream" ) );
            m_streamParams.erase( it );
            ret = segmentStart( true );
        }
    }
    return ret;
}

int CameraOutput::stopSegment() {
    return segmentEnd();
}

std::future< int > CameraOutput::allowRecord() {
    assert( !m_worker.joinable() );
    auto promise = std::make_shared< std::promise< int > >();

    if( m_worker.joinable() ) {
        Log::warn( cc::warn( "Already started" ) );
        promise->set_value( AVERROR_UNKNOWN );
        return promise->get_future();
    }

    if( m_streamParams.empty() ) {
        Log::error( cc::error( "Add streams first!" ) );
        promise->set_value( AVERROR_UNKNOWN );
        return promise->get_future();
    }

    int ret = segmentStart( true );
    if( ret == AVERROR( EINVAL ) ) {
        // We cannot define a reason of the error, but perhaps it's because of
        // unsupported audio codec, try to record without audio stream.
        auto it =
            std::find_if( m_streamParams.begin(), m_streamParams.end(), []( const TStreamParams::value_type& params ) {
                return params.second.codecParams.get()->codec_type == AVMEDIA_TYPE_AUDIO;
            } );
        if( it != m_streamParams.end() ) {
            Log::info( cc::info( "Trying to record without audio stream" ) );
            m_streamParams.erase( it );
            ret = segmentStart( true );
        }
    }
    if( ret < 0 ) {
        promise->set_value( ret );
        return promise->get_future();
    }

    m_worker = std::thread( [this, promise] {
        Log::debug( cc::debug( "Starting camera output: thread id: " ) + cc::threadId() );
        m_shouldCancel = false;
        // @todo: reset other fields
        int ret = AVERROR_UNKNOWN;
        while( !m_shouldCancel ) {
            AVPacketPrototype packet;
            if( !m_buffer.getFrontElement( packet ) ) {
                // Wait for next portion of frames.
                std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
                continue;
            }
            ret = processPacket( packet );
            if( ret < 0 ) {
                break;
            }
        }
        promise->set_value( ret );
    } );
    return promise->get_future();
}

void CameraOutput::stop() {
    Log::debug( cc::debug( "Stopping camera output" ) );
    m_shouldStop = true;
}

void CameraOutput::cancel() {
    LOGGER_TRACE_RAW( cc::trace( "Stopping camera output thread" ) );
    if( m_worker.joinable() ) {
        m_shouldCancel = true;
        m_worker.join();
    }
}

StorageSharedPtr CameraOutput::getStorage() const {
    return m_storage;
}

void CameraOutput::addVideoStream( int streamIndex, const AVStream& inputStream, Resolution outputResolution ) {
    assert( !m_worker.joinable() );
    StreamParams streamParams;
    streamParams.codecParams = AVCodecParamsWrapper( inputStream.codecpar );
    streamParams.codecParams.get()->codec_tag = 0;
    streamParams.inputTimeBase = inputStream.time_base;
    streamParams.outputResolution = outputResolution;
    m_streamParams.emplace( streamIndex, std::move( streamParams ) );
}

void CameraOutput::addAudioStream( int streamIndex, const AVStream& inputStream ) {
    assert( !m_worker.joinable() );
    StreamParams streamParams;
    streamParams.codecParams = AVCodecParamsWrapper( inputStream.codecpar );
    streamParams.codecParams.get()->codec_tag = 0;
    streamParams.inputTimeBase = inputStream.time_base;
    m_streamParams.emplace( streamIndex, std::move( streamParams ) );
}

void CameraOutput::receivePacket( const AVPacketPrototype& packet ) {
    // Ignore packets from other streams.
    if( !checkStreamIndex( packet.getPacket().stream_index ) ) {
        return;
    }

    if( !m_buffer.appendElement( packet.clone() ) ) {
        Log::warn( cc::warn( "Camera output buffer is full, skipping frame" ) );
    }
}

int CameraOutput::segmentStart( bool reset ) {
    if( reset ) {
        m_segmentCount = 0;
    }
    auto date = GregorianCalendar::Date( std::chrono::system_clock::now() ).toString();
    std::string fileName = m_fileNameStart + "_" + date + "_rec_" + std::to_string( m_recordNumber ) + "_seg_" +
                           std::to_string( m_segmentCount ) + "." + m_fileExtension;
    auto fileSender = m_storage->createFileSender( fileName );
    if( !fileSender ) {
        LOGGER_ERROR( "file sender creating error" );
        return AVERROR_UNKNOWN;
    }
    auto newSegment = std::make_unique< Segment >( fileName, m_outputMuxer, fileSender );
    int ret = newSegment->init( m_memBuffer, m_streamParams );
    if( ret < 0 ) {
        fileSender->setFileValidity( /*isValid*/ false );
        return ret;
    }
    if( newSegment->referenceStream < 0 ) {
        LOGGER_ERROR( "no video stream" );
        return AVERROR_UNKNOWN;
    }
    m_currentSegment = std::move( newSegment );
    return ret;
}

int CameraOutput::segmentEnd() {
    int ret = m_currentSegment->finalize();
    m_currentSegment->deinit();
    ++m_segmentCount;
    auto& storageVolume = m_storage->getStorageVolume();
    storageVolume += m_currentSegment->segmentSize;
    Log::trace( "Used storage space : " + std::to_string( storageVolume ) );
    m_currentSegment->segmentSize = 0;
    if( storageVolume >= m_storage->getMaxStorageVolume() ) {
        Log::error( cc::error( "Storage is full" ) );
        return -1;
    }
    return ret;
}

bool CameraOutput::checkStreamIndex( int index ) const {
    return m_streamParams.find( index ) != m_streamParams.end();
}

void CameraOutput::fixPacketTimestamps( AVPacket& packet, int64_t lastDts ) const {
    // there is a case, when the new incoming packet has a smaller DTS value, then the previous packet's DTS
    // it causes this error in FFMPEG: "Application provided invalid, non monotonically increasing dts to muxer"
    // which leads to the stop of stream recording, so the following code corrects the DTS's value to prevent this
    // similar fixes: https://github.com/horgh/videostreamer/issues/5,
    //                https://github.com/FFmpeg/FFmpeg/blob/master/fftools/ffmpeg.c, search "Invalid DTS:"

    const int64_t nextDts = lastDts + 1;
    const int64_t oldDts = packet.dts;
    const int64_t oldPts = packet.pts;

    if( packet.pts != AV_NOPTS_VALUE && packet.pts >= packet.dts ) {
        packet.pts = std::max( packet.pts, nextDts );
    }

    packet.dts = nextDts;

    LOGGER_TRACE_RAW( cc::trace( "Fixing packet's timestamps in CameraOutput for '" ) + cc::note( m_fileNameStart ) +
                      cc::trace( "', pts: " + std::to_string( oldPts ) + " => " + std::to_string( packet.pts ) +
                                 " dts: " + std::to_string( oldDts ) + " => " + std::to_string( packet.dts ) ) );
}

CameraOutput::Segment::Segment( std::string fileName, std::string outputMuxer, FileSenderSharedPtr sender )
: fileName( std::move( fileName ) )
, outputMuxer( std::move( outputMuxer ) )
, fileSender( std::move( sender ) ) {}

CameraOutput::Segment::~Segment() {
    deinit();
}

int CameraOutput::Segment::init( unsigned char* memBuffer, const TStreamParams& params ) {
    outputFormat = av_guess_format( outputMuxer.c_str(), nullptr, nullptr );
    if( !outputFormat ) {
        Log::error( cc::error( "Could not find format " + outputMuxer ) );
        return AVERROR_UNKNOWN;
    }
    outputFormat->flags |= AVFMT_NOFILE;

    outputFormatContext = avformat_alloc_context();
    if( !outputFormatContext ) {
        Log::error( cc::error( "Could not create output context" ) );
        return AVERROR_UNKNOWN;
    }
    outputFormatContext->oformat = outputFormat;

    int ret = AVERROR_UNKNOWN;
    for( const auto& p : params ) {
        const int streamIndex = p.first;
        const StreamParams& streamParams = p.second;

        AVStream* outStream = avformat_new_stream( outputFormatContext, nullptr );
        if( !outStream ) {
            Log::error( cc::error( "Failed allocating output stream" ) );
            return AVERROR_UNKNOWN;
        }

        const AVCodecParameters* inputCodecParams = streamParams.codecParams.get();
        const Resolution streamResolution( inputCodecParams->width, inputCodecParams->height );
        if( inputCodecParams->codec_type == AVMEDIA_TYPE_VIDEO && streamResolution != streamParams.outputResolution ) {
            try {
                m_videoConverter = std::make_unique< VideoConverter >( *inputCodecParams,
                                                                       streamParams.inputTimeBase,
                                                                       *outputFormatContext,
                                                                       streamParams.outputResolution );
                ret = m_videoConverter->fillOutputStreamCodecParams( *outStream );
                outStream->time_base = streamParams.inputTimeBase;
            }
            catch( const FFmpegHelpers::Exception& ex ) {
                Log::error( cc::error( ex.what() ) );
                ret = ex.errorCode;
                m_videoConverter.reset();
            }
        }
        else if( inputCodecParams->codec_type == AVMEDIA_TYPE_AUDIO && inputCodecParams->codec_id != AV_CODEC_ID_AAC ) {
            /* Set the sample rate for the container. */
            outStream->time_base.den = inputCodecParams->sample_rate;
            outStream->time_base.num = 1;

            try {
                m_audioConverter =
                    std::make_unique< AudioConverter >( *inputCodecParams, *outputFormatContext, streamIndex );
                ret = m_audioConverter->fillOutputStreamCodecParams( *outStream );
            }
            catch( const FFmpegHelpers::Exception& ex ) {
                Log::error( cc::error( ex.what() ) );
                ret = ex.errorCode;
                m_audioConverter.reset();
            }
        }
        else {
            ret = avcodec_parameters_copy( outStream->codecpar, inputCodecParams );
        }

        if( ret < 0 ) {
            Log::error( cc::error( "Failed to copy codec parameters: " + FFmpegHelpers::avStringError( ret ) ) );
            return ret;
        }
        outStream->codecpar->codec_tag = 0;

        if( outStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ) {
            referenceStream = streamIndex;
        }
    }

    outputIoContext = avio_alloc_context(
        memBuffer, outputBufferSize, 1 /*write flag*/, this /*user data*/, nullptr, writePacket, nullptr );
    if( !outputIoContext ) {
        Log::error( cc::error( "avio_alloc_context() failed" ) );
        return AVERROR_UNKNOWN;
    }
    outputFormatContext->pb = outputIoContext;
    outputFormatContext->flags |= AVFMT_FLAG_CUSTOM_IO;

    FFmpegHelpers::AVDictionaryWrapper opts;
    ret = av_dict_set( &opts, "movflags", "frag_keyframe+empty_moov", 0 );
    if( ret < 0 ) {
        Log::error( cc::error( "Could not set fragmented output format." ) );
    }
    ret = avformat_write_header( outputFormatContext, &opts );
    if( ret < 0 ) {
        Log::error( cc::error( "Error occurred when opening output file: " + FFmpegHelpers::avStringError( ret ) ) );
    }
    else {
        Log::trace( cc::trace( "Created new segment " + fileName ) );
    }

    return ret;
}

void CameraOutput::Segment::deinit() {
    avio_context_free( &outputIoContext );

    if( outputFormatContext && !( outputFormat->flags & AVFMT_NOFILE ) )
        avio_closep( &outputFormatContext->pb );
    avformat_free_context( outputFormatContext );
    outputFormatContext = nullptr;
}

int CameraOutput::Segment::finalize() {
    if( m_videoConverter ) {
        m_videoConverter->flush();
    }
    if( m_audioConverter ) {
        m_audioConverter->flush();
    }

    int ret = av_write_trailer( outputFormatContext );
    if( ret < 0 ) {
        Log::error( cc::error( "Could not write trailer: " + FFmpegHelpers::avStringError( ret ) ) );
    }
    else {
        Log::trace( cc::trace( "Finalized segment: " + fileName ) );
    }
    fileSender->finalizeFile();
    return ret;
}

int CameraOutput::Segment::write( AVPacket& packet ) {
    if( !recordingStartStored ) {
        storeStartTimestamp();
        recordingStartStored = true;
    }

    int ret = 0;
    if( m_videoConverter && referenceStream == packet.stream_index ) {
        ret = m_videoConverter->process( packet );
    }
    else if( m_audioConverter && referenceStream != packet.stream_index ) {
        ret = m_audioConverter->process( packet );
    }
    else {
        ret = av_interleaved_write_frame( outputFormatContext, &packet );
    }
    return ret;
}

int CameraOutput::Segment::writePacket( void* opaque, uint8_t* buf, int bufSize ) {
    auto segment = reinterpret_cast< Segment* >( opaque );
    segment->segmentSize += bufSize;
    if( segment->fileSender->sendData( buf, bufSize ) ) {
        return bufSize;
    }
    return AVERROR_UNKNOWN;
}

void CameraOutput::Segment::storeStartTimestamp() {
    using namespace std::chrono;
    const auto timeSinceEpoch = system_clock::now().time_since_epoch();
    const int64_t timestamp = duration_cast< seconds >( timeSinceEpoch ).count();

    Json dynamicMetaData;
    dynamicMetaData[ "timestamp" ] = timestamp;
    fileSender->setDynamicMetaData( dynamicMetaData );
}

}  // namespace HubPlatform
