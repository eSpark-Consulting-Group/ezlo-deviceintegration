#include "tiny/tiny_consolecolors.h"
#include "tiny/tiny_consolecolors_filters.h"

#include <gtest/gtest.h>

namespace tiny {
namespace UT {

class RpcMessageFiltersTests : public ::testing::Test {};

TEST_F( RpcMessageFiltersTests, noFilters ) {
    auto& rpcFiltering = cc::RpcMessageFiltersSingleton::instance();

    const unsigned originalSize = 5000;
    std::string message( originalSize, 'a' );

    std::string truncated;
    EXPECT_FALSE( rpcFiltering.processFilters( message, truncated ) ) << "Should not be truncated";
    EXPECT_TRUE( truncated.empty() ) << "If false returned, should be empty";
}

static const int suffixSize = 30;  //!< Limit of suffix added to truncated strings

TEST_F( RpcMessageFiltersTests, truncateMessagesBySize ) {
    auto& rpcFiltering = cc::RpcMessageFiltersSingleton::instance();

    std::shared_ptr< cc::RpcMessageFiltersBySize > sizeFilter = std::make_shared< cc::RpcMessageFiltersBySize >();
    rpcFiltering.pushBack( sizeFilter );

    const unsigned originalSize = 5000;
    std::string message( originalSize, 'a' );
    size_t sizeBefore = message.size();
    EXPECT_EQ( originalSize, sizeBefore );

    std::string truncated;
    EXPECT_TRUE( rpcFiltering.processFilters( message, truncated ) )
        << "Message should be truncated to " << sizeFilter->sizeLimit() << " plus suffix";

    EXPECT_GE( truncated.size(), sizeFilter->sizeLimit() );
    EXPECT_LE( truncated.size(), sizeFilter->sizeLimit() + suffixSize );

    truncated.clear();
    // rpc_log: truncate messages passed to rpc_log
    truncated = cc::rpc_log( cc::e_rpc_log_type_t::erpcltt_ws_rx, "type", "descr", message, false );
    EXPECT_GE( truncated.size(), sizeFilter->sizeLimit() );
    EXPECT_LE( truncated.size(), sizeFilter->sizeLimit() + 100 ) << "rpc_log output should be truncated";
}

TEST_F( RpcMessageFiltersTests, truncateMessagesBySize_pushFront ) {
    auto& rpcFiltering = cc::RpcMessageFiltersSingleton::instance();

    std::shared_ptr< cc::RpcMessageFiltersBySize > sizeFilter = std::make_shared< cc::RpcMessageFiltersBySize >();
    rpcFiltering.pushBack( sizeFilter );

    std::shared_ptr< cc::RpcMessageFiltersBySize > sizeFilter2 = std::make_shared< cc::RpcMessageFiltersBySize >();
    sizeFilter2->sizeLimit( 100 );
    rpcFiltering.pushFront( sizeFilter2 );

    const unsigned originalSize = 5000;
    std::string message( originalSize, 'a' );
    size_t sizeBefore = message.size();
    EXPECT_EQ( originalSize, sizeBefore );

    std::string truncated;
    EXPECT_TRUE( rpcFiltering.processFilters( message, truncated ) )
        << "Message should be truncated to " << sizeFilter->sizeLimit() << " plus suffix";

    EXPECT_GE( truncated.size(), sizeFilter2->sizeLimit() );
    EXPECT_LE( truncated.size(), sizeFilter2->sizeLimit() + suffixSize )
        << "Should be truncated by first filter, not the last one";

    truncated.clear();
    // rpc_message: e_rpc_log_type_t { erpcltt_ws_tx, erpcltt_ws_rx }
    truncated = cc::rpc_log( cc::e_rpc_log_type_t::erpcltt_ws_rx, "type", "descr", message, false );
    EXPECT_GE( truncated.size(), sizeFilter2->sizeLimit() );
    EXPECT_LE( truncated.size(), sizeFilter2->sizeLimit() + 100 ) << "rpc_log output should be truncated";
}

}  // namespace UT
}  // namespace tiny
