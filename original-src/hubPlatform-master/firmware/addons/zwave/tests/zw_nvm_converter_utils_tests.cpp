#include "zwave/zwave_addon.h"
#include "zwave/zwave_client.h"
#include "zwave/resources/zwave_resources.h"
#include "zwave/znet.h"
#include "zwave/znet_factory.h"

#include "zwave/tests/mocks/znet_mock.h"

#include "bus/base_bus.h"
#include "service_interfaces/bus_wrapper.h"
#include "plugin_events_subscriber/plugin_events_subscriber.h"
#include "common_utils/cpp14.h"
#include "test_utils/test_utils.h"
#include "database_client/dbqueryparser.h"
#include "provision_data_utils/provision_data_manager.h"
#include "jsonrpc/jsonrpc.h"
#include "common_utils/hub_info_helper.h"
#include "common_utils/firmware_info_helper.h"
#include "zwave/anti_theft_helper.h"
#include "zwave/zw_nvm_converter_utils/zw_nvm_converter_utils.h"

#include "test_utils/temporary_folder_wrapper.h"
#include "test_utils/temporary_file_wrapper.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <sp.h>

#include <type_traits>
#include <fstream>

extern "C" {
#include "zw_nvm_converter/nvmconv.h"
}

namespace HubPlatform {
namespace Zwave {
namespace UT {

using namespace ::testing;

constexpr const char* testJsonImage6_7 = R"(
{
    "backupInfo":{
        "backupFormatVersion":1,
        "sourceProtocolVersion":"05.03.00",
        "sourceAppVersion":"05.50.00",
        "date":"Thu Jul 15 19:04:18 2021\n"
    },
    "zwController":{
        "nodeId":0,
        "ownHomeId":"0x7AEC9FCB",
        "learnedHomeId":"0x00000000",
        "lastUsedNodeId":4,
        "staticControllerNodeId":0,
        "controllerConfiguration":40,
        "cmdClassList":[
            94,
            134,
            114,
            90,
            89,
            133,
            115,
            86,
            33,
            152,
            34,
            143,
            138,
            139,
            43,
            239,
            96,
            132,
            129,
            122,
            142
        ],
        "nodeTable":[
            {
                "nodeId":1,
                "pendingUpdate":false,
                "pendingDiscovery":false,
                "routeSlaveSuc":false,
                "controllerSucUpdateIndex":255,
                "neighbours":[
                    2,
                    3,
                    4
                ],
                "nodeInfo":{
                    "capability":211,
                    "security":22,
                    "reserved":1,
                    "generic":2,
                    "specific":1
                },
                "routeCache":{
                    "applock":false,
                    "LWR":{
                        "routecacheLineConf":0,
                        "repeaters":[
                            0,
                            0,
                            0,
                            0
                        ]
                    },
                    "NLWR":{
                        "routecacheLineConf":0,
                        "repeaters":[
                            0,
                            0,
                            0,
                            0
                        ]
                    }
                }
            },
            {
                "nodeId":2,
                "pendingUpdate":false,
                "pendingDiscovery":false,
                "routeSlaveSuc":false,
                "controllerSucUpdateIndex":254,
                "neighbours":[
                    1
                ],
                "nodeInfo":{
                    "capability":211,
                    "security":156,
                    "reserved":0,
                    "generic":17,
                    "specific":1
                },
                "routeCache":{
                    "applock":false,
                    "LWR":{
                        "routecacheLineConf":0,
                        "repeaters":[
                            0,
                            0,
                            0,
                            0
                        ]
                    },
                    "NLWR":{
                        "routecacheLineConf":0,
                        "repeaters":[
                            0,
                            0,
                            0,
                            0
                        ]
                    }
                }
            },
            {
                "nodeId":3,
                "pendingUpdate":false,
                "pendingDiscovery":false,
                "routeSlaveSuc":false,
                "controllerSucUpdateIndex":255,
                "neighbours":[
                    1
                ],
                "nodeInfo":{
                    "capability":211,
                    "security":22,
                    "reserved":1,
                    "generic":2,
                    "specific":1
                },
                "routeCache":{
                    "applock":false,
                    "LWR":{
                        "routecacheLineConf":3,
                        "repeaters":[
                            254,
                            0,
                            0,
                            0
                        ]
                    },
                    "NLWR":{
                        "routecacheLineConf":2,
                        "repeaters":[
                            254,
                            0,
                            0,
                            0
                        ]
                    }
                }
            },
            {
                "nodeId":4,
                "pendingUpdate":false,
                "pendingDiscovery":false,
                "routeSlaveSuc":false,
                "controllerSucUpdateIndex":254,
                "neighbours":[
                    1
                ],
                "nodeInfo":{
                    "capability":211,
                    "security":156,
                    "reserved":1,
                    "generic":16,
                    "specific":1
                },
                "routeCache":{
                    "applock":false,
                    "LWR":{
                        "routecacheLineConf":3,
                        "repeaters":[
                            254,
                            0,
                            0,
                            0
                        ]
                    },
                    "NLWR":{
                        "routecacheLineConf":0,
                        "repeaters":[
                            0,
                            0,
                            0,
                            0
                        ]
                    }
                }
            }
        ],
        "sucState":{
            "lastIndex":255,
            "updateNodeList":[
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                },
                {
                    "nodeId":0,
                    "changeType":0,
                    "nodeInfo":[

                    ]
                }
            ]
        },
        "applicationData":"OQQFDggH"
    },
    "appConfig":{
        "watchdogStarted":0,
        "powerLevelNormal":[
            0,
            0,
            0
        ],
        "powerLevelLow":[
            0,
            0,
            0
        ]
    }
}
)";

constexpr const char* testJsonImage7_12 = R"(
{
  "backupInfo":{
    "backupFormatVersion":1,
    "sourceProtocolVersion":"07.12.00",
    "sourceAppVersion":"07.00.00",
    "date":"2021-07-15T16-22-52Z"
  },
  "zwController":{
    "nodeId":1,
    "ownHomeId":"0x7AEC9FCB",
    "learnedHomeId":"0x7AEC9FCB",
    "lastUsedNodeId":4,
    "staticControllerNodeId":0,
    "controllerConfiguration":40,
    "systemState":0,
    "cmdClassList":[
      94,
      134,
      114,
      90,
      89,
      133,
      115,
      86,
      33,
      152,
      34,
      143,
      138,
      139,
      43,
      239,
      96,
      132,
      129,
      122,
      142
    ],
    "nodeTable":[
      {
        "nodeId":1,
        "virtualNode":false,
        "pendingUpdate":false,
        "pendingDiscovery":false,
        "routeSlaveSuc":false,
        "controllerSucUpdateIndex":255,
        "neighbours":[
          2,
          3,
          4
        ],
        "nodeInfo":{
          "capability":211,
          "security":22,
          "reserved":1,
          "generic":2,
          "specific":1
        },
        "routeCache":{
          "applock":false,
          "LWR":{
            "routecacheLineConf":0,
            "repeaters":[
              0,
              0,
              0,
              0
            ]
          },
          "NLWR":{
            "routecacheLineConf":0,
            "repeaters":[
              0,
              0,
              0,
              0
            ]
          }
        }
      },
      {
        "nodeId":2,
        "virtualNode":false,
        "pendingUpdate":false,
        "pendingDiscovery":false,
        "routeSlaveSuc":false,
        "controllerSucUpdateIndex":254,
        "neighbours":[
          1
        ],
        "nodeInfo":{
          "capability":211,
          "security":156,
          "reserved":0,
          "generic":17,
          "specific":1
        },
        "routeCache":{
          "applock":false,
          "LWR":{
            "routecacheLineConf":0,
            "repeaters":[
              0,
              0,
              0,
              0
            ]
          },
          "NLWR":{
            "routecacheLineConf":0,
            "repeaters":[
              0,
              0,
              0,
              0
            ]
          }
        }
      },
      {
        "nodeId":3,
        "virtualNode":false,
        "pendingUpdate":false,
        "pendingDiscovery":false,
        "routeSlaveSuc":false,
        "controllerSucUpdateIndex":255,
        "neighbours":[
          1
        ],
        "nodeInfo":{
          "capability":211,
          "security":22,
          "reserved":1,
          "generic":2,
          "specific":1
        },
        "routeCache":{
          "applock":false,
          "LWR":{
            "routecacheLineConf":3,
            "repeaters":[
              254,
              0,
              0,
              0
            ]
          },
          "NLWR":{
            "routecacheLineConf":2,
            "repeaters":[
              254,
              0,
              0,
              0
            ]
          }
        }
      },
      {
        "nodeId":4,
        "virtualNode":false,
        "pendingUpdate":false,
        "pendingDiscovery":false,
        "routeSlaveSuc":false,
        "controllerSucUpdateIndex":254,
        "neighbours":[
          1
        ],
        "nodeInfo":{
          "capability":211,
          "security":156,
          "reserved":1,
          "generic":16,
          "specific":1
        },
        "routeCache":{
          "applock":false,
          "LWR":{
            "routecacheLineConf":3,
            "repeaters":[
              254,
              0,
              0,
              0
            ]
          },
          "NLWR":{
            "routecacheLineConf":0,
            "repeaters":[
              0,
              0,
              0,
              0
            ]
          }
        }
      }
    ],
    "sucState":{
      "lastIndex":255,
      "updateNodeList":[
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        },
        {
          "nodeId":0,
          "changeType":0,
          "nodeInfo":[
          ]
        }
      ]
    },
    "applicationData":"OQQFDggH"
  },
  "appConfig":{
    "rfRegion":0,
    "txPower":0,
    "power0dbmMeasured":0
  }
}
)";

class ZwNvmConverterUtilsTestSuite : public Test {
public:
    ZwNvmConverterUtilsTestSuite() {
        HubPlatform::Environment::setProvisionDataFolder( m_tmpDir.getPath() );
    }

    void setRegion( const char* region ) {
        ProvisionDataUtils::ProvisionData data = HubPlatform::ProvisionDataUtils::getManager().getProvisionData();
        ++data.configVersion;
        data.zwaveRegion = region;

        HubPlatform::ProvisionDataUtils::getManager().update( data );
    }

    Json makeJsonFromBinFile( zw_nvm_format format, const std::string& binFilePath ) const {
        HubPlatform::Tests::TemporaryFileWrapper file;

        EXPECT_TRUE( zw_nvm_export( format, binFilePath.c_str(), file.getFilePath().c_str(), true ) );

        std::ifstream stream( file.getFilePath() );
        std::string content( ( std::istreambuf_iterator< char >( stream ) ), std::istreambuf_iterator< char >() );

        return Json::parse( content );
    }

    HubPlatform::Tests::TemporaryFileWrapper makeBinFileFormTestJson( zw_nvm_format format ) const {
        HubPlatform::Tests::TemporaryFileWrapper file;

        EXPECT_TRUE( zw_nvm_import( format, m_initialJsonFile.getFilePath().c_str(), file.getFilePath().c_str() ) );

        return file;
    }

    const std::string& getInitialJsonImageFilePath() const {
        return m_initialJsonFile.getFilePath();
    }

protected:
    void SetUp() override {
        std::ofstream stream( m_initialJsonFile.getFilePath() );
        stream << testJsonImage6_7;
    }

protected:
    HubPlatform::Tests::TemporaryFolderWrapper m_tmpDir;
    HubPlatform::Tests::TemporaryFileWrapper m_initialJsonFile;
};

class ZwNvmConverterUtilsTestSuiteWithRegionParams
: public ZwNvmConverterUtilsTestSuite,
  public ::testing::WithParamInterface< std::tuple< const char*, int > > {};

TEST_F( ZwNvmConverterUtilsTestSuite, convertImage ) {
    const auto imageFile = makeBinFileFormTestJson( ZW_NVM_FORMAT_STATIC_6_7 );
    HubPlatform::Tests::TemporaryFileWrapper resultImageFile7_12;
    EXPECT_FALSE( Zwave::NvmConverterUtils::convertImage( Zwave::NvmConverterUtils::Format::Static6_8,
                                                          Zwave::NvmConverterUtils::Format::Bridge7_12,
                                                          imageFile.getFilePath(),
                                                          resultImageFile7_12.getFilePath(),
                                                          nullptr ) );
    EXPECT_FALSE( Zwave::NvmConverterUtils::convertImage( Zwave::NvmConverterUtils::Format::Static6_8,
                                                          Zwave::NvmConverterUtils::Format::Bridge7_12,
                                                          imageFile.getFilePath(),
                                                          resultImageFile7_12.getFilePath(),
                                                          nullptr ) );

    // without modification
    {
        EXPECT_TRUE( Zwave::NvmConverterUtils::convertImage( Zwave::NvmConverterUtils::Format::Static6_7,
                                                             Zwave::NvmConverterUtils::Format::Bridge7_12,
                                                             imageFile.getFilePath(),
                                                             resultImageFile7_12.getFilePath(),
                                                             nullptr ) );

        auto json = makeJsonFromBinFile( ZW_NVM_FORMAT_BRIDGE_7_12, resultImageFile7_12.getFilePath() );
        auto expectedJson = Json::parse( testJsonImage7_12 );
        expectedJson[ "backupInfo" ][ "date" ] = json[ "backupInfo" ][ "date" ];
        EXPECT_EQ( json, expectedJson );
    }

    // with modification
    {
        EXPECT_TRUE( Zwave::NvmConverterUtils::convertImage( Zwave::NvmConverterUtils::Format::Static6_7,
                                                             Zwave::NvmConverterUtils::Format::Bridge7_12,
                                                             imageFile.getFilePath(),
                                                             resultImageFile7_12.getFilePath(),
                                                             []( Json& json ) {
                                                                 json[ "appConfig" ][ "rfRegion" ] = 2;
                                                                 json[ "appConfig" ][ "txPower" ] = 100;
                                                                 return true;
                                                             } ) );

        auto json = makeJsonFromBinFile( ZW_NVM_FORMAT_BRIDGE_7_12, resultImageFile7_12.getFilePath() );
        auto expectedJson = Json::parse( testJsonImage7_12 );
        expectedJson[ "backupInfo" ][ "date" ] = json[ "backupInfo" ][ "date" ];
        expectedJson[ "appConfig" ][ "rfRegion" ] = 2;
        expectedJson[ "appConfig" ][ "txPower" ] = 100;
        EXPECT_EQ( json, expectedJson );
    }

    // double conversion
    {
        HubPlatform::Tests::TemporaryFileWrapper convertedImageFile;
        EXPECT_TRUE( Zwave::NvmConverterUtils::convertImage( Zwave::NvmConverterUtils::Format::Static6_7,
                                                             Zwave::NvmConverterUtils::Format::Bridge6_8,
                                                             imageFile.getFilePath(),
                                                             convertedImageFile.getFilePath(),
                                                             nullptr ) );
        EXPECT_TRUE( Zwave::NvmConverterUtils::convertImage( Zwave::NvmConverterUtils::Format::Bridge6_8,
                                                             Zwave::NvmConverterUtils::Format::Static6_7,
                                                             convertedImageFile.getFilePath(),
                                                             convertedImageFile.getFilePath(),
                                                             nullptr ) );

        std::ifstream originalStream6_7( imageFile.getFilePath(), std::ifstream::binary );
        std::ifstream convertedStream6_7( convertedImageFile.getFilePath(), std::ifstream::binary );
        EXPECT_TRUE( std::equal( std::istreambuf_iterator< char >( originalStream6_7 ),
                                 std::istreambuf_iterator< char >(),
                                 std::istreambuf_iterator< char >( convertedStream6_7 ) ) );
    }
}

TEST_P( ZwNvmConverterUtilsTestSuiteWithRegionParams, convertVeraImageToActualFormat ) {
    setRegion( std::get< 0 >( GetParam() ) );

    MockFunction< void( const Json& ) > jsonHandler;

    EXPECT_CALL( jsonHandler, Call( _ ) ).WillOnce( [this]( const Json& json ) {
        EXPECT_EQ( json[ "appConfig" ][ "rfRegion" ], std::get< 1 >( GetParam() ) );
    } );

    const auto imageFile = makeBinFileFormTestJson( ZW_NVM_FORMAT_STATIC_6_7 );
    HubPlatform::Tests::TemporaryFileWrapper resultImageFile;

    EXPECT_TRUE( Zwave::NvmConverterUtils::convertVeraImageToActualFormat(
        imageFile.getFilePath(), resultImageFile.getFilePath(), jsonHandler.AsStdFunction() ) );

    HubPlatform::Tests::TemporaryFileWrapper expectedResultImageFile;
    EXPECT_TRUE( Zwave::NvmConverterUtils::convertImage( Zwave::NvmConverterUtils::Format::Static6_7,
                                                         Zwave::NvmConverterUtils::Format::Bridge7_12,
                                                         imageFile.getFilePath(),
                                                         expectedResultImageFile.getFilePath(),
                                                         []( Json& json ) {
                                                             json[ "appConfig" ][ "rfRegion" ] =
                                                                 std::get< 1 >( GetParam() );
                                                             return true;
                                                         } ) );

    std::ifstream resultStream( resultImageFile.getFilePath(), std::ifstream::binary );
    std::ifstream expectedResultStream( expectedResultImageFile.getFilePath(), std::ifstream::binary );
    EXPECT_TRUE( std::equal( std::istreambuf_iterator< char >( resultStream ),
                             std::istreambuf_iterator< char >(),
                             std::istreambuf_iterator< char >( expectedResultStream ) ) );
}

INSTANTIATE_TEST_CASE_P( DifferentLocales,
                         ZwNvmConverterUtilsTestSuiteWithRegionParams,
                         ::testing::Values( std::make_tuple( "us", 1 ),
                                            std::make_tuple( "US", 1 ),
                                            std::make_tuple( "eu", 0 ),
                                            std::make_tuple( "EU", 0 ),
                                            std::make_tuple( "unknown", 0 ) ) );

}  // namespace UT
}  // namespace Zwave
}  // namespace HubPlatform
