#pragma once

struct AVCodecParameters;

namespace HubPlatform {

class AVCodecParamsWrapper final {
public:
    AVCodecParamsWrapper() = default;
    explicit AVCodecParamsWrapper( const AVCodecParameters* sourceCodecParams );
    ~AVCodecParamsWrapper();
    AVCodecParamsWrapper( const AVCodecParamsWrapper& );
    AVCodecParamsWrapper& operator=( const AVCodecParamsWrapper& ) = delete;
    AVCodecParamsWrapper( AVCodecParamsWrapper&& right ) noexcept;
    AVCodecParamsWrapper& operator=( AVCodecParamsWrapper&& right ) noexcept;

    AVCodecParameters* get();
    const AVCodecParameters* get() const;

private:
    AVCodecParameters* m_codecParams = nullptr;
};
}  // namespace HubPlatform
