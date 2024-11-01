#pragma once

#include "database_client/idbclient.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {
namespace UT {

class IDbClientMock : public IDbClient {
public:
    MOCK_METHOD3( insert, void( const std::string&, const Json&, const bool ) );
    MOCK_METHOD5( update, void( const std::string&, const Json&, const Json&, bool, bool ) );
    MOCK_METHOD3( update, void( const std::string&, const Json&, std::function< void( Json& ) > ) );
    MOCK_CONST_METHOD6(
        findAll,
        std::vector< Json >( const std::string&, const db::Query&, const Json*, int, int, const bool ) );
    MOCK_METHOD2( count, size_t( const std::string&, const Json& ) );
    MOCK_METHOD2( has, bool( const std::string&, const Json& ) );
    MOCK_METHOD4( remove, void( const std::string&, const Json&, bool, const bool ) );
    MOCK_METHOD2( capped, void( const std::string&, int ) );
    MOCK_CONST_METHOD1( exist, bool( const std::string& ) );
    MOCK_METHOD1( create, void( const std::string& ) );
    MOCK_METHOD1( clear, void( const std::string& ) );
    MOCK_METHOD1( drop, void( const std::string& ) );
    MOCK_METHOD0( clean, void() );
    MOCK_METHOD0( fsync, void() );
    MOCK_METHOD0( reset, void() );
    MOCK_METHOD0( lock, void() );
    MOCK_METHOD0( unlock, void() );
    MOCK_METHOD0( resetChangesCounter, void() );
    MOCK_CONST_METHOD1( size, std::size_t( const std::string& ) );
    MOCK_METHOD1( connect, void( const int ) );
    MOCK_CONST_METHOD0( connected, bool() );
    MOCK_METHOD1( fixBrokenPrimaryKeys, void( const std::string& ) );
    MOCK_CONST_METHOD1( getPrimaryKeys, std::vector< std::string >( const std::string& ) );
    MOCK_CONST_METHOD1( getAllCollectionsNames, std::vector< std::string >( const bool ) );
    MOCK_CONST_METHOD5(
        forEachNth,
        void( const std::string&, const Json&, const std::function< void( Json, const size_t ) >&, size_t, size_t ) );
};

}  // namespace UT
}  // namespace HubPlatform
