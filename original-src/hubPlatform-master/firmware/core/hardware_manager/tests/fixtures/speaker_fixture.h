#pragma once

#include "service_interfaces/tests/fakes/fake_service.h"
#include "hardware_manager/tests/mocks/speaker_environment_mock.h"

#include "hardware_manager/speaker/speaker.h"

#include <gtest/gtest.h>

#include <map>
#include <mutex>

namespace HubPlatform {
namespace HardwareManager {

class SpeakerFixture : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

    static void SetUpTestCase();

    void finishForkProcess( pid_t pid, Process::Forker::Status status, int exitCode, const std::string& error );

    void addForkProcess( pid_t pid, Process::Forker::OnProcessCompletion callBack );

protected:
    std::unique_ptr< Services::UT::FakeService > m_service;

    Speaker::EnvironmentMock m_environment;

    using FakeForksCollection = std::map< pid_t, Process::Forker::OnProcessCompletion >;
    FakeForksCollection m_fakeForks;
    std::mutex m_fakeForksMutex;

    std::unique_ptr< Speaker::Speaker > m_speaker;
};

}  // namespace HardwareManager
}  // namespace HubPlatform