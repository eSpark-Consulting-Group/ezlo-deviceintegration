#pragma once

#include <functional>

namespace HubPlatform {

class IPostponedMethodManager {
public:
    enum class MethodType { StartLiveRtsp, CreateSavingToCloud, StartSavingToCloud, StartParticleRecording };

    using PostponedMethod = std::function< void() >;

    virtual void runAllMethods() = 0;
    virtual void addMethod( MethodType, PostponedMethod ) = 0;
    virtual void removeMethod( MethodType ) = 0;

    virtual ~IPostponedMethodManager() = default;
};

}  // namespace HubPlatform
