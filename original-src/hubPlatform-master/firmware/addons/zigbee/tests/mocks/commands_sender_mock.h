#pragma once

#include "zigbee/utils/commands_sender_interface.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {
namespace Zigbee {
namespace UT {

class CommandsSenderMock : public CommandsSenderInterface {
public:
    CommandsSenderMock() {}

    MOCK_METHOD5( sendCommand, void( uint16_t, uint8_t, uint16_t, const std::string&, const Json& ) );
    MOCK_METHOD6( setAttributeValue, void( uint16_t, uint8_t, uint16_t, uint16_t, uint8_t, const Json& ) );
    MOCK_METHOD3( sendBindingCommand, void( uint16_t, uint8_t, uint16_t ) );
    MOCK_METHOD6( sendConfigureAttributeReportingCommand,
                  void( uint16_t, uint8_t, uint16_t, uint16_t, bool, uint16_t ) );
    MOCK_METHOD10( sendConfigureAttributeReportingCommand,
                   void( uint16_t,
                         uint8_t,
                         uint16_t,
                         uint16_t,
                         AttributeValueType,
                         const uint16_t,
                         const uint16_t,
                         const Json&,
                         bool,
                         uint16_t ) );
    MOCK_METHOD6( requestAttribute, bool( uint16_t, uint8_t, uint16_t, uint16_t, bool, uint16_t ) );
    MOCK_METHOD6( requestAttributes,
                  bool( uint16_t, uint8_t, uint16_t, const std::vector< uint16_t >&, bool, uint16_t ) );
    MOCK_METHOD7( discoverAttributes, bool( uint16_t, uint8_t, uint16_t, uint16_t, uint8_t, bool, uint16_t ) );
    MOCK_METHOD8( discoverCommandsReceived,
                  bool( uint16_t, uint8_t, uint16_t, uint16_t, uint8_t, CommandDirection, bool, uint16_t ) );
    MOCK_METHOD8( discoverCommandsGenerated,
                  bool( uint16_t, uint8_t, uint16_t, uint16_t, uint8_t, CommandDirection, bool, uint16_t ) );
};

}  // namespace UT
}  // namespace Zigbee
}  // namespace HubPlatform
