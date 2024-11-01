#include "../data_sender.h"
#include "fakes/fake_udp_server.h"
#include "resources/test_resources.h"

#include <gtest/gtest.h>
#include <vector>

/*
TESTPLAN
DONE:     1.   Check Send Data
TODO:     1.1. Check valid string
TODO:     1.2. Check invalid string
TODO:     1.3. Check send interval status with valid string
TODO:     1.4. Check memory anomaly
TODO:     1.4. Check load average anomaly
*/

class DataSenderTestSuite : public ::testing::Test {
protected:
    void SetUp() override {
        using namespace HubPlatform::Metrics;
        m_sender = std::make_shared< DataSender >( std::make_shared< InternalMetrics >() );
        m_udpServer.initUdpServer();
    }
    void TearDown() override {
        m_udpServer.stopServer();
    }

public:
    using DataSenderPtr = std::shared_ptr< HubPlatform::Metrics::DataSender >;
    DataSenderPtr m_sender;
    FakeUdpServer m_udpServer;
    const char* m_udpAddr = "127.0.0.1";

private:
};

TEST_F( DataSenderTestSuite, checkDataSend ) {
    int length = 0;

    m_sender->setServerPublicKey( HubPlatform::Metrics::UT::serverPublicKey, HubPlatform::Metrics::STRING_SOURCE );
    m_sender->setClientPrivateKey( HubPlatform::Metrics::UT::clientPrivateKey, HubPlatform::Metrics::STRING_SOURCE );
    m_sender->setClientCertificate( HubPlatform::Metrics::UT::clientCrt, HubPlatform::Metrics::STRING_SOURCE );
    m_sender->setServerUrlAndPort( m_udpAddr, std::vector< size_t >( PORT, PORT ) );

    bool isReaded = m_udpServer.readServer( length );
    bool isTrue = m_sender->sendData( std::string{ HubPlatform::Metrics::UT::jsonData } );

    ASSERT_TRUE( isTrue );
}
