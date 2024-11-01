#pragma once

#include <json_utils/json_fwd.h>

#include <memory>
#include <functional>
#include <chrono>
#include <string>

namespace HubPlatform {

class AVPacketPrototype;

class FileSender;
using FileSenderSharedPtr = std::shared_ptr< FileSender >;

class IFileRemover;
using FileRemoverSharedPtr = std::shared_ptr< IFileRemover >;

class FolderRemover;
using FolderRemoverPtr = std::shared_ptr< FolderRemover >;

class Storage;
using StorageSharedPtr = std::shared_ptr< Storage >;

class Camera;
using CameraPtr = std::unique_ptr< Camera >;

class CameraInput;
using CameraInputUniquePtr = std::unique_ptr< CameraInput >;

class CameraOutput;
using CameraOutputUniquePtr = std::unique_ptr< CameraOutput >;

class LiveCloudOutput;
using LiveCloudOutputUniquePtr = std::unique_ptr< LiveCloudOutput >;

class PacketHandler;
using PacketHandlerSharedPtr = std::shared_ptr< PacketHandler >;

class FrameHandler;
using FrameHandlerSharedPtr = std::shared_ptr< FrameHandler >;

class CameraArchiver;
using CameraArchiverUniquePtr = std::unique_ptr< CameraArchiver >;

class AVFrameWrapper;
using FrameWrapperSharedPtr = std::shared_ptr< AVFrameWrapper >;

class CameraSnapshoter;
using CameraSnapshoterUniquePtr = std::unique_ptr< CameraSnapshoter >;

enum class StorageType { Unknown, Local, PlainHttp };

using ResultCallback = std::function< void( const Json& ) >;

using FilePath = std::string;
using SaveFileCallback =
    std::function< void( const FilePath&, ResultCallback, FileRemoverSharedPtr, const Json& metaData ) >;

class CameraException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

enum class StreamError { CAMERA_STREAM_CLOSED, CAMERA_UNREACHABLE, CAMERA_PACKET_DELAY, CAMERA_ACCUMULATED_DELAY };

constexpr int DefaultRtspServerPort = 5554;
constexpr int DefaultRtspServerPacketBufferSize = 500;
constexpr int DefaultLocalStorageMaxSizeMb = 10;
constexpr int DefaultRecordingBufferSize = 1000;
constexpr int DefaultPreRecordingBufferSize = 2000;
constexpr int DefaultSnapshoterBufferSize = 100;
constexpr int DefaultSnapshoterQueueLimit = 20;
constexpr int DefaultSnapshoterTimeout = 10;
constexpr int DefaultStreamingMaxDelayMsec = 1000;
constexpr int DefaultAccumulatedDelayThresholdMsec = 5000;
constexpr int DefaultSnapshotTimeout = 20;

}  // namespace HubPlatform
