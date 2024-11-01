#pragma once

#include <memory>
#include <queue>
#include <vector>
#include <mutex>

namespace HubPlatform {
namespace Network {
namespace Mobile {

class InterfaceNumberManager;

class InterfaceNumber final {
    friend class InterfaceNumberManager;

public:
    ~InterfaceNumber();
    int getInterfaceNumber() const;

private:
    InterfaceNumber( InterfaceNumberManager& interfaceNumberManager, bool interfaceEmbedded );

private:
    InterfaceNumberManager& m_interfaceNumberManager;
    bool m_interfaceEmbedded;
    int m_interfaceNumber;
};

class InterfaceNumberManager final {
    friend class InterfaceNumber;

public:
    InterfaceNumberManager();

    std::unique_ptr< InterfaceNumber > allocateInterfaceNumber( bool interfaceEmbedded );

private:
    int acquireInterfaceNumber( bool interfaceEmbedded );
    void releaseInterfaceNumber( int interfaceNumber, bool interfaceEmbedded );

private:
    std::priority_queue< int, std::vector< int >, std::greater< int > > m_interfaceNumbersQueue;
    std::mutex m_interfaceNumbersQueueMutex;
};

}  // namespace Mobile
}  // namespace Network
}  // namespace HubPlatform
