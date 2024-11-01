#pragma once
// Extracted from action.h/cpp

#include "scenes_manager/scenes_engine/actions/action.h"
#include "extensions_manager/remote_interface/em_remote_interface.h"

namespace SceneEngine {

class RunCustomScriptAction final : public Action {
public:
    RunCustomScriptAction( HubPlatform::ExtensionsManager::RemoteInterface& extensionManager, InitData&& initData );

private:
    HubPlatform::ExtensionsManager::RemoteInterface& m_extensionManager;
    HubPlatform::ExtensionsManager::RemoteInterface::SharedRemoteAnonymousPlugin m_remoteAnonymousPlugin;
};

class RunPluginScriptAction final : public Action {
public:
    RunPluginScriptAction( HubPlatform::ExtensionsManager::RemoteInterface& extensionManager, InitData&& initData );

private:
    HubPlatform::ExtensionsManager::RemoteInterface& m_extensionManager;
};

}  // namespace SceneEngine