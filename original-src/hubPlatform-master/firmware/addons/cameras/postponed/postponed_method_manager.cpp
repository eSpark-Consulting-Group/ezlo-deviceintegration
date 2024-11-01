#include "cameras/postponed/postponed_method_manager.h"

namespace HubPlatform {

void PostponedMethodManager::runAllMethods() {
    for( const auto& method : m_postponedMethods ) {
        method.second();
    }
    m_postponedMethods.clear();
}

void PostponedMethodManager::addMethod( MethodType type, PostponedMethod method ) {
    m_postponedMethods.emplace( type, method );
}

void PostponedMethodManager::removeMethod( MethodType type ) {
    m_postponedMethods.erase( type );
}

}  // namespace HubPlatform
