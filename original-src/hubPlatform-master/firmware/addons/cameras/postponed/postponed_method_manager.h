#pragma once

#include "cameras/postponed/postponed_method_manager_interface.h"

#include <map>

namespace HubPlatform {

class PostponedMethodManager : public IPostponedMethodManager {
public:
    void runAllMethods() override;

    void addMethod( MethodType type, PostponedMethod method ) override;

    void removeMethod( MethodType type ) override;

private:
    using PostponedMethods = std::map< MethodType, PostponedMethod >;

    PostponedMethods m_postponedMethods;
};

}  // namespace HubPlatform
