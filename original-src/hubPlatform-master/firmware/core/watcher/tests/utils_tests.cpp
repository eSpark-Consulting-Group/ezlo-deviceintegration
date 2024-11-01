#include "watcher/utils/utils.h"

#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"
#include "json_utils/json_utils.h"
#include <gtest/gtest.h>

#include <string>

/*

TESTPLAN

DONE    1. Utils tests
DONE        1.1 Parse OpenWRT 'ps' output
DONE        1.2 Parse proc fs status file

*/

namespace HubPlatform {
namespace UT {

class WatcherUtilsTestSuite : public ::testing::Test {};

TEST_F( WatcherUtilsTestSuite, parseOpenwrtPsOutput ) /* 1.1 */ {
    // clang-format off
    std::string output = \
R"(  PID USER       VSZ STAT COMMAND
    7 root         0 DW   [kworker/u8:0+cf]
11147 root       896 D    ip route
<invalid line>

)";
    // clang-format on

    auto result = Watcher::Utils::parseOpenwrtPsOutput( output );
    ASSERT_EQ( result.size(), 2 );

    {
        auto& element = result.at( 0 );
        EXPECT_EQ( element.pid, 7 );
        EXPECT_EQ( element.state, "DW" );
        EXPECT_EQ( element.cmd, "[kworker/u8:0+cf]" );
    }

    {
        auto& element = result.at( 1 );
        EXPECT_EQ( element.pid, 11147 );
        EXPECT_EQ( element.state, "D" );
        EXPECT_EQ( element.cmd, "ip route" );
    }
}

TEST_F( WatcherUtilsTestSuite, parseProcFsStatus ) /* 1.2 */ {
    // clang-format off
    std::string output = \
R"(Name:   processname
Umask:  0022
State:  D (disk sleep)
Tgid:   3508
Ngid:   0
Pid:    3508
PPid:   2
TracerPid:      0
Uid:    0       0       0       0
Gid:    0       0       0       0
FDSize: 32
Groups:  
NStgid: 3508
NSpid:  3508
NSpgid: 0
NSsid:  0
Threads:        1
SigQ:   1/3952
SigPnd: 0000000000000000
ShdPnd: 0000000000000000
SigBlk: 0000000000000000
SigIgn: ffffffffffffffff
SigCgt: 0000000000000000
CapInh: 0000000000000000
CapPrm: 0000003fffffffff
CapEff: 0000003fffffffff
CapBnd: 0000003fffffffff
CapAmb: 0000000000000000
NoNewPrivs:     0
Seccomp:        0
Speculation_Store_Bypass:       unknown
Cpus_allowed:   f
Cpus_allowed_list:      0-3
Mems_allowed:   1
Mems_allowed_list:      0
voluntary_ctxt_switches:        2
nonvoluntary_ctxt_switches:     0
)";
    // clang-format on

    auto result = Watcher::Utils::parseProcFsStatus( output );

    EXPECT_EQ( result.pid, 3508 );
    EXPECT_EQ( result.state, "D" );
    EXPECT_EQ( result.cmd, "processname" );
}

}  // namespace UT
}  // namespace HubPlatform
