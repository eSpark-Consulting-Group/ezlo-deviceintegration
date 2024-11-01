#pragma once

#include "device_firmware_update_manager/device_firmware_update_manager.h"
#include "http/http.h"

#include <exception>
#include <thread>

namespace HubPlatform {

class DefaultDownloader : public DeviceFirmwareUpdateManager::Downloader {
public:
    using ExtendedLink = DeviceFirmwareUpdateManager::ExtendedLink;
    using Path = DeviceFirmwareUpdateManager::Path;
    using Exception = std::runtime_error;

    DefaultDownloader( const ExtendedLink& sourceLink, const Path& destinationPath, ResultCallback resultcallback );

    ~DefaultDownloader();

    bool isOk() const override;

    uint8_t getProgress() const override;

private:
    bool isLink( const DeviceFirmwareUpdateManager::Link& link );
    void initHttpReceivingCallback();
    void initHttpConnectionClosedCallback( ResultCallback resultcallback );
    void initHttpDownloadingProgressCallback( const Path& destinationPath, ResultCallback resultcallback );

private:
    enum class ChecksumDownloadStatus { Done, Waiting, NoDownload };

    CommonHttp::Http m_http;
    CommonHttp::Http::RequestId m_imageRequestId;
    CommonHttp::Http::RequestId m_checksumRequestId;
    bool m_imageDownloadFinished = false;
    ChecksumDownloadStatus m_checksumDownloadStatus = ChecksumDownloadStatus::NoDownload;
    std::string m_checksumFilePath;
    std::atomic< uint8_t > m_progress;
    uint64_t m_downloadedSize = 0;
    std::atomic_bool m_isOk;
    mutable std::mutex m_mutex;
    std::thread m_thread;

    static const int ResponseTimeout = 60 * 1000;
    static const int ConnectionTimeout = 30 * 1000;
    static const int RedirectionNumber = 0;
};

}  // namespace HubPlatform
