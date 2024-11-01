
#include "hardware_manager/tests/fixtures/speaker_fixture.h"

#include <gmock/gmock-actions.h>

namespace HubPlatform {
namespace HardwareManager {

using ::testing::ReturnRef;

void SpeakerFixture::SetUp() {
    Test::SetUp();
    m_service = std::make_unique< Services::UT::FakeService >();

    EXPECT_CALL( m_environment, getService() ).WillRepeatedly( ReturnRef( *m_service ) );

    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );

    m_speaker = std::make_unique< Speaker::Speaker >( m_environment );
}

void SpeakerFixture::TearDown() {
    m_speaker.reset();
    m_service.reset();

    Log::setLocalLogLevel( Log::PriorityLevel::DEBUG );
}

void SpeakerFixture::SetUpTestCase() {}

void SpeakerFixture::addForkProcess( pid_t pid, Process::Forker::OnProcessCompletion callBack ) {
    std::unique_lock< std::mutex > scopeLock( m_fakeForksMutex );
    m_fakeForks[ pid ] = std::move( callBack );
}

void SpeakerFixture::finishForkProcess( pid_t pid,
                                        Process::Forker::Status status,
                                        int exitCode,
                                        const std::string& error ) {
    Process::Forker::OnProcessCompletion callBack;
    {
        std::unique_lock< std::mutex > scopeLock( m_fakeForksMutex );
        auto itFind = m_fakeForks.find( pid );
        if( itFind == m_fakeForks.end() ) {
            throw std::runtime_error( std::string( "Cannot find information for PID in fixture: " ) +
                                      std::to_string( pid ) );
        }

        callBack = itFind->second;

        m_fakeForks.erase( itFind );
    }

    callBack( pid, status, exitCode, error );
}

}  // namespace HardwareManager
}  // namespace HubPlatform
