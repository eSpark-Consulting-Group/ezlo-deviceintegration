#pragma once
#include <string>
#include <stdexcept>
#include <functional>

extern "C" {
#include <libavformat/avformat.h>
}

namespace HubPlatform {
namespace FFmpegHelpers {

std::string avStringError( int errnum );
std::string formatAvErrorMessage( const std::string& messageBase, const std::string& avError );

void logAvError( const std::string& messageBase, int errnum );
void logAvError( const std::string& message );

class Exception : public std::exception {
public:
    explicit Exception( std::string text );
    Exception( std::string text, int errc );
    const char* what() const noexcept override;

public:
    const int errorCode;

private:
    std::string m_text;
};

class AVDictionaryWrapper {
public:
    explicit AVDictionaryWrapper( AVDictionary* opts = nullptr )
    : m_opts( opts ) {}
    ~AVDictionaryWrapper() {
        av_dict_free( &m_opts );
    }

    operator AVDictionary*() {
        return m_opts;
    }

    AVDictionary** operator&() {
        return &m_opts;
    }

    explicit operator bool() const {
        return m_opts != nullptr;
    }

private:
    AVDictionary* m_opts;
};

using ErrorCallback = std::function< void( int ) >;

}  // namespace FFmpegHelpers
}  // namespace HubPlatform
