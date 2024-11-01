#include "device_firmware_update_manager/private/default_downloader.h"
#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"
#include "http/http_events.h"
#include "crypto_utils/digests.h"
#include "system_utils/filesystem.h"
#include <fstream>
#include <thread>

namespace HubPlatform {

const int DefaultDownloader::ResponseTimeout;
const int DefaultDownloader::ConnectionTimeout;
const int DefaultDownloader::RedirectionNumber;

DefaultDownloader::DefaultDownloader( const ExtendedLink& sourceLink,
                                      const Path& destinationPath,
                                      ResultCallback resultcallback )
: m_progress( 0 )
, m_isOk( true ) {
    if( !isLink( sourceLink.image ) ) {
        m_thread = std::thread( [this, sourceLink, destinationPath, resultcallback]() {
            resultcallback( *this );

            LOGGER_DEBUG( "copy " + sourceLink.image + " -> " + destinationPath );

            m_isOk = filesystem::copyFile( sourceLink.image, destinationPath );
            m_progress = 100;

            resultcallback( *this );
        } );
        return;
    }

    initHttpReceivingCallback();
    initHttpConnectionClosedCallback( resultcallback );
    initHttpDownloadingProgressCallback( destinationPath, resultcallback );

    CommonHttp::HttpOptions httpOptions;
    httpOptions.url = sourceLink.image;
    httpOptions.type = "GET";
    httpOptions.handler = "empty";
    httpOptions.timeoutMs = ResponseTimeout;
    httpOptions.connectTimeoutMs = ConnectionTimeout;
    httpOptions.redirection = RedirectionNumber;
    httpOptions.receivedDataFilePath = destinationPath;
    m_imageRequestId = m_http.request( "downloader", httpOptions );

    if( !sourceLink.checksum.empty() ) {
        m_checksumDownloadStatus = ChecksumDownloadStatus::Waiting;
        m_checksumFilePath = filesystem::getPath( destinationPath ) + "/checksum.sh1";
        httpOptions.url = sourceLink.checksum;
        httpOptions.receivedDataFilePath = m_checksumFilePath;
        m_checksumRequestId = m_http.request( "downloader", httpOptions );
    }
}

bool DefaultDownloader::isOk() const {
    return m_isOk;
}

uint8_t DefaultDownloader::getProgress() const {
    return m_progress;
}

bool DefaultDownloader::isLink( const DeviceFirmwareUpdateManager::Link& link ) {
    const std::string& linkMarker = "http";
    return link.substr( 0, linkMarker.size() ) == linkMarker;
}

void DefaultDownloader::initHttpReceivingCallback() {
    using namespace CommonHttp::Resources;
    auto eventName = HttpEvents::HttpDataReceived;
    m_http.on( eventName, [this]( const std::string& /*eventHandler*/, CommonHttp::EventPtr eventPtr ) {
        if( !eventPtr.get() )
            return;
        auto message = eventPtr->toJson();
        auto code = json_get_int( message, "code" );
        std::lock_guard< std::mutex > lock( m_mutex );
        if( m_isOk ) {
            m_isOk = code == 200;
        }
    } );
}

void DefaultDownloader::initHttpConnectionClosedCallback( ResultCallback resultcallback ) {
    using namespace CommonHttp::Resources;
    m_http.on( HttpEvents::HttpProgressEvent,
               [this, resultcallback]( const std::string& /*eventHandler*/, CommonHttp::EventPtr eventPtr ) {
                   if( !eventPtr.get() )
                       return;
                   auto message = eventPtr->toJson();
                   auto requestId = json_get_str( message, "id" );
                   std::unique_lock< std::mutex > lock( m_mutex );
                   if( requestId == m_imageRequestId ) {
                       lock.unlock();
                       m_progress = json_get_int( message, "progress" );
                       if( m_progress != 100 ) {
                           resultcallback( *this );
                       }
                   }
               } );
}

void DefaultDownloader::initHttpDownloadingProgressCallback( const Path& destinationPath,
                                                             ResultCallback resultcallback ) {
    using namespace CommonHttp::Resources;
    m_http.on(
        HttpEvents::HttpConnectionClosed,
        [this, resultcallback, destinationPath]( const std::string& /*eventHandler*/, CommonHttp::EventPtr eventPtr ) {
            if( !eventPtr.get() )
                return;
            auto message = eventPtr->toJson();
            int reasonCode;
            json_get( message, std::vector< Json >{ "reason", "code" }, reasonCode );
            if( m_isOk ) {
                m_isOk = static_cast< CommonHttp::Reason >( reasonCode ) == CommonHttp::Reason::NONE;
            }
            std::unique_lock< std::mutex > lock( m_mutex );
            auto requestId = json_get_str( message, "id" );
            if( requestId == m_imageRequestId ) {
                m_imageDownloadFinished = true;
            }
            else if( requestId == m_checksumRequestId ) {
                m_checksumDownloadStatus = ChecksumDownloadStatus::Done;
            }
            else {
                Log::error( cc::error( "invalid request id" ) );
            }
            if( m_imageDownloadFinished && m_checksumDownloadStatus == ChecksumDownloadStatus::NoDownload ) {
                lock.unlock();
                resultcallback( *this );
            }
            else if( m_imageDownloadFinished && m_checksumDownloadStatus == ChecksumDownloadStatus::Done ) {
                lock.unlock();
                std::ifstream binaryFile( destinationPath, std::ios::binary );
                if( !binaryFile.is_open() ) {
                    Log::error( cc::error( "file not opened: " + destinationPath ) );
                    m_isOk = false;
                    resultcallback( *this );
                    return;
                }
                std::ifstream checksumFile( m_checksumFilePath, std::ios::binary );
                if( !checksumFile.is_open() ) {
                    Log::error( cc::error( "file not opened: " + m_checksumFilePath ) );
                    m_isOk = false;
                    resultcallback( *this );
                    return;
                }
                std::string checksumRef;
                checksumFile >> checksumRef;
                auto checksum = CryptoUtils::Digests::getInstance().sha1( binaryFile );
                if( checksum != checksumRef ) {
                    Log::error( cc::error( "checksum test is failed for file: " + destinationPath ) );
                    m_isOk = false;
                }
                resultcallback( *this );
            }
        } );
}

DefaultDownloader::~DefaultDownloader() {
    if( m_http.joinable() ) {
        m_http.join();
    }
    if( m_thread.joinable() ) {
        m_thread.join();
    }
}

}  // namespace HubPlatform
