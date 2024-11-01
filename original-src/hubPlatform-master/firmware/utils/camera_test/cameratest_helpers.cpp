#include "camera_test/cameratest_helpers.h"
#include "camera_test/command.h"

#include "database_client/lmdb_client.h"
#include "database_client/dbqueryparser.h"
#include "database_client/dbtools.h"

#include <map>
#include <algorithm>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>

struct StreamData {
    StreamData( std::string localRtspUrl, std::string cameraRtspUrl )
    : localRtspUrl( std::move( localRtspUrl ) )
    , cameraRtspUrl( std::move( cameraRtspUrl ) ) {}

    std::string localRtspUrl;
    std::string cameraRtspUrl;
};

bool urlHasValidIp( const std::string& url ) {
    auto itIpEnd = std::find( url.rbegin(), url.rend(), ':' );
    auto itIpStart = std::find( itIpEnd, url.rend(), '/' );

    bool hasIp = false;

    if( itIpEnd != url.rend() && itIpStart != url.rend() ) {
        auto inputItIpEnd = ( itIpEnd + 1 ).base();
        std::string ip( itIpStart.base(), inputItIpEnd );

        unsigned char buffer[ sizeof( struct in6_addr ) ];
        hasIp = inet_pton( AF_INET, ip.c_str(), buffer ) == 1 || inet_pton( AF_INET6, ip.c_str(), buffer ) == 1;
    }

    return hasIp;
}

std::map< std::string, StreamData > collectStreamItems( const IDbClient& dbClient ) {
    std::map< std::string, StreamData > streamIdDataMap;

    const std::vector< Json > cameraStreamItems = dbClient.findAll( HaItemsDBName, db::Query{}, nullptr );
    for( const Json& cameraStreamItem : cameraStreamItems ) {
        const Json& itemValue = cameraStreamItem[ "value" ];
        if( itemValue.is_object() ) {
            const Json& localRtspUrl = itemValue[ "localRtspUrl" ];
            if( localRtspUrl.is_string() ) {
                std::string itemIdStr = cameraStreamItem[ "_id" ].get< std::string >();
                std::string localRtspUrlStr = localRtspUrl.get< std::string >();

                if( urlHasValidIp( localRtspUrlStr ) ) {
                    streamIdDataMap.emplace( std::move( itemIdStr ), StreamData( std::move( localRtspUrlStr ), "" ) );
                }
                else {
                    std::cout << "Item with _id \"" << itemIdStr
                              << "\" has no valid IP address in the \"localRtspUrl\", skipping the stream\n";
                }
            }
        }
    }

    return streamIdDataMap;
}

std::vector< std::string > collectActiveStreams( const IDbClient& dbClient ) {
    auto streamIdDataMap = collectStreamItems( dbClient );

    const Json requiredFields = JSON( "streamId" << 0 << "rtspUrl" << 0 );
    const std::vector< Json > allCameraStreams = dbClient.findAll( CameraStreamsDBName, db::Query(), &requiredFields );
    for( const Json& cameraStream : allCameraStreams ) {
        const std::string streamId = cameraStream[ "streamId" ].get< std::string >();
        auto it = streamIdDataMap.find( streamId );
        if( it != streamIdDataMap.end() ) {
            std::string cameraRtspUrl = cameraStream[ "rtspUrl" ].get< std::string >();

            StreamData& streamData = it->second;
            streamData.cameraRtspUrl = std::move( cameraRtspUrl );
        }
    }

    std::vector< std::string > activeStreams;
    activeStreams.reserve( streamIdDataMap.size() * 2 );
    for( auto& streamIdDataPair : streamIdDataMap ) {
        StreamData& streamData = streamIdDataPair.second;
        activeStreams.push_back( std::move( streamData.localRtspUrl ) );
        activeStreams.push_back( std::move( streamData.cameraRtspUrl ) );
    }

    return activeStreams;
}

std::string findUrlForTesting( const Command& command ) {
    std::string urlForTesting;

    if( !command.streamUrl.empty() ) {
        urlForTesting = command.streamUrl;
    }
    else {
        LMDBClient dbClient( getStorageFilePath(), getTemporaryStorageFilePath() );
        const std::vector< std::string > activeStreams = collectActiveStreams( dbClient );

        if( command.streamIdx < 0 || command.streamIdx >= activeStreams.size() )
            throw std::out_of_range( "Specified stream index is out of range of active streams" );

        urlForTesting = activeStreams[ command.streamIdx ];
    }

    return urlForTesting;
}

std::vector< char > buildFfmpegLogMessage( const char* format, va_list args ) {
    // the impl taken from "Log" class
    std::vector< char > message( 1024 );
    while( true ) {
        va_list args2;
        va_copy( args2, args );
        int res = vsnprintf( message.data(), message.size(), format, args2 );
        if( ( res >= 0 ) && ( res < static_cast< int >( message.size() ) ) ) {
            va_end( args2 );
            break;
        }
        size_t size;
        if( res < 0 ) {
            size = message.size() * 2;
        }
        else {
            size = static_cast< size_t >( res ) + 1;
        }

        message.clear();
        message.resize( size );
        va_end( args2 );
    }

    return message;
}
