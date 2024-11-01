#pragma once

#include <vector>
#include <chrono>

extern "C" {
#include <libavformat/avformat.h>
}

namespace HubPlatform {
namespace Cameras {

enum class Status { RUN, FINISHED };

class Packet {
public:
    Packet()
    : isKey( false )
    , isPreCached( false ) {}
    std::vector< uint8_t > data;
    std::chrono::nanoseconds timestamp;
    Status status;
    int streamIndex;
    bool isKey;
    bool isPreCached;
};

}  // namespace Cameras

class AVPacketPrototype {
public:
    ///
    /// \brief AVPacketPrototype creates AVPacket with default values, i.e. not valid
    ///
    AVPacketPrototype();

    ///
    /// \brief AVPacketPrototype takes ownership of AVPacket, that contains
    /// initialized data, i.e. valid
    /// \param packet
    ///
    explicit AVPacketPrototype( const AVPacket& packet );

    /// If packet is valid, unrefs AVPacket
    ~AVPacketPrototype();

    AVPacketPrototype( const AVPacketPrototype& ) = delete;
    AVPacketPrototype( AVPacketPrototype&& r ) noexcept;
    AVPacketPrototype& operator=( AVPacketPrototype&& r ) noexcept;

    ///
    /// \brief clone creates new AVPacketPrototype that refers the same AVPacket
    /// \return AVPacketPrototype instance
    ///
    AVPacketPrototype clone() const;

    bool isValid() const;
    AVPacket& getPacket();
    const AVPacket& getPacket() const;

private:
    AVPacket m_packet;
    bool m_isValid;
};

class AVPacketWrapper {
public:
    AVPacketWrapper();
    ~AVPacketWrapper();

    AVPacketWrapper( AVPacketWrapper&& r ) noexcept;
    AVPacketWrapper& operator=( AVPacketWrapper&& r ) noexcept;

    AVPacket* getPacket();

private:
    AVPacket* m_packet;
};

}  // namespace HubPlatform
