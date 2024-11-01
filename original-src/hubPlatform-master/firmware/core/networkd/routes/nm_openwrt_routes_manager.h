#pragma once

#include "networkd/routes/nm_routes_manager.h"

namespace HubPlatform {
namespace Network {

class OpenwrtRoutesManager : public RoutesManager {
public:
    class ActionHandlerInterface {
    public:
        virtual std::string readRoutesTable() = 0;
        virtual bool applyRouteAction( const std::string& action ) = 0;
    };

    class Route {
    public:
        static Route parse( std::string route );

        bool isDefault = false;
        std::string device;
        std::string source;
        std::string gateway;
        std::string metric;
        std::string proto;
        std::string routeString;
    };

    OpenwrtRoutesManager();
    OpenwrtRoutesManager( std::unique_ptr< ActionHandlerInterface > actionHandler );

    std::string getDefaultRouteDevice() override;
    std::string checkAndFixRoutes( const std::vector< Json >& interfaces ) override;

    std::vector< Route > readRoutes();

private:
    std::string getDefaultRouteDevice( const std::vector< Route >& routes );

private:
    std::unique_ptr< ActionHandlerInterface > m_actionHandler;
};

}  // namespace Network
}  // namespace HubPlatform
