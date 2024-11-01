
#include "config_reader/tests/config_reader_factory.h"
#include "json_utils/json_wrapper.h"
#include "system_utils/environment.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <unordered_map>
#include <thread>

namespace HubPlatform {
namespace UT {

class ConfigReaderTestSuite : public testing::Test, public ConfigReaderFactory {};

TEST_F( ConfigReaderTestSuite, intField ) {
    const auto configReader = createConfigReader( []( const std::string& ) { return JSON( "intField" << 1 ).dump(); } );

    EXPECT_EQ( configReader.getInt( "source/intField" ), 1 );
    EXPECT_EQ( configReader.getFloat( "source/intField" ), 1.f );

    EXPECT_THROW( configReader.getInt( "source/invalidField" ), ConfigReader::ReadingError );

    EXPECT_THROW( configReader.getString( "source/intField" ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getBool( "source/intField" ), ConfigReader::ReadingError );
    EXPECT_THROW( ConfigReader::getArraySize( ConfigReader::Path( configReader, "source/intField" ) ),
                  ConfigReader::ReadingError );
}

TEST_F( ConfigReaderTestSuite, floatField ) {
    const auto configReader =
        createConfigReader( []( const std::string& ) { return JSON( "floatField" << 1.6f ).dump(); } );

    EXPECT_EQ( configReader.getInt( "source/floatField" ), 1 );
    EXPECT_EQ( configReader.getFloat( "source/floatField" ), 1.6f );

    EXPECT_THROW( configReader.getFloat( "source/invalidField" ), ConfigReader::ReadingError );

    EXPECT_THROW( configReader.getString( "source/floatField" ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getBool( "source/floatField" ), ConfigReader::ReadingError );
    EXPECT_THROW( ConfigReader::getArraySize( ConfigReader::Path( configReader, "source/floatField" ) ),
                  ConfigReader::ReadingError );
}

TEST_F( ConfigReaderTestSuite, stringField ) {
    const auto configReader =
        createConfigReader(
            []( const std::string& ) {
                return JSON( "stringField"
                             << "1" )
                    .dump();
            } );

    EXPECT_EQ( configReader.getString( "source/stringField" ), "1" );

    EXPECT_THROW( configReader.getString( "source/invalidField" ), ConfigReader::ReadingError );

    EXPECT_THROW( configReader.getInt( "source/stringField" ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getFloat( "source/stringField" ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getBool( "source/stringField" ), ConfigReader::ReadingError );
    EXPECT_THROW( ConfigReader::getArraySize( ConfigReader::Path( configReader, "source/stringField" ) ),
                  ConfigReader::ReadingError );
}

TEST_F( ConfigReaderTestSuite, stringArrayField ) {
    const char fieldName[] = "source/stringArrayField";
    const size_t arraySize = 5;
    const auto getStringValue = []( size_t i ) -> std::string {
        return std::string( "string" ).append( std::to_string( i ) );
    };
    const auto configReader = createConfigReader( [&getStringValue, &fieldName]( const std::string& ) {
        JsonArray jStringArray;
        for( size_t i = 0; i < arraySize; ++i ) {
            jStringArray.push_back( getStringValue( i ) );
        }
        return JSON( "stringArrayField" << jStringArray ).dump();
    } );
    const auto path = ConfigReader::Path( configReader, fieldName );
    EXPECT_EQ( configReader.getArraySize( path ), arraySize );

    const auto stringVec = configReader.getStringArray( path );
    for( size_t i = 0; i < arraySize; ++i ) {
        EXPECT_EQ( stringVec[ i ].compare( getStringValue( i ) ), 0 );
    }

    EXPECT_THROW( configReader.getStringArray( ConfigReader::Path( configReader, "source/invalidField" ) ),
                  ConfigReader::ReadingError );

    EXPECT_THROW( configReader.getInt( fieldName ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getFloat( fieldName ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getBool( fieldName ), ConfigReader::ReadingError );
}

TEST_F( ConfigReaderTestSuite, boolField ) {
    const auto configReader =
        createConfigReader( []( const std::string& ) { return JSON( "boolField" << true ).dump(); } );

    EXPECT_TRUE( configReader.getBool( "source/boolField" ) );

    EXPECT_THROW( configReader.getBool( "source/invalidField" ), ConfigReader::ReadingError );

    EXPECT_THROW( configReader.getInt( "source/boolField" ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getString( "source/boolField" ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getFloat( "source/boolField" ), ConfigReader::ReadingError );
    EXPECT_THROW( ConfigReader::getArraySize( ConfigReader::Path( configReader, "source/boolField" ) ),
                  ConfigReader::ReadingError );
}

TEST_F( ConfigReaderTestSuite, configPath ) {
    const auto configReader = createConfigReader( []( const std::string& source ) {
        EXPECT_EQ( source, "source" );
        return JSON( "root" << JSON( "child" << JSON( "grandchild"
                                                      << "value"
                                                      << "grandchild2"
                                                      << "value2" ) ) )
            .dump();
    } );

    EXPECT_EQ( configReader.getString( "source/root/child/grandchild" ), "value" );
    EXPECT_EQ( configReader.getString( "source/root/child/grandchild2" ), "value2" );

    EXPECT_THROW( configReader.getString( "source/root" ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getString( "source/root/child" ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getString( "source/root/child/grandchild/" ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getString( "source/root/child//grandchild" ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getString( "source/root/child//grandchild/value" ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getString( "source/root/child\\grandchild" ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getString( "source/root/child /grandchild" ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getString( "source/Root/child/grandchild" ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getString( "source/" ), ConfigReader::ReadingError );
}

TEST_F( ConfigReaderTestSuite, hasValue ) {
    const auto configReader = createConfigReader( []( const std::string& source ) {
        if( source == "source1" ) {
            return JSON( "floatField" << 1.6f << "intField" << 1 << "boolField" << true << "stringField"
                                      << "str"
                                      << "stringArray" << JSON_ARRAY( 1 << 2 ) )
                .dump();
        }

        if( source == "source2" ) {
            return JSON( "root" << JSON( "child" << JSON( "grandchild"
                                                          << "value"
                                                          << "grandchild2"
                                                          << "value2" ) ) )
                .dump();
        }

        return std::string();
    } );

    EXPECT_TRUE( configReader.hasValue( "source1/floatField" ) );
    EXPECT_TRUE( configReader.hasValue( "source1/boolField" ) );
    EXPECT_TRUE( configReader.hasValue( "source1/intField" ) );
    EXPECT_TRUE( configReader.hasValue( "source1/stringField" ) );
    EXPECT_TRUE( configReader.hasValue( "/source1/stringField" ) );
    EXPECT_TRUE( configReader.hasValue( "source1/stringArray" ) );

    EXPECT_FALSE( configReader.hasValue( "//source1/intField" ) );
    EXPECT_FALSE( configReader.hasValue( "source1" ) );
    EXPECT_FALSE( configReader.hasValue( "source1/" ) );

    EXPECT_TRUE( configReader.hasValue( "source2/root/child/grandchild" ) );
    EXPECT_TRUE( configReader.hasValue( "source2/root/child/grandchild2" ) );
    EXPECT_TRUE( configReader.hasValue( "/source2/root/child/grandchild2" ) );

    EXPECT_FALSE( configReader.hasValue( "source2/root/child/grandchild2/" ) );
    EXPECT_FALSE( configReader.hasValue( "source2/root/Child/grandchild2" ) );
    EXPECT_FALSE( configReader.hasValue( "source2/root/child/grandchild3" ) );
    EXPECT_FALSE( configReader.hasValue( "source2/root/child" ) );
    EXPECT_FALSE( configReader.hasValue( "source2" ) );
    EXPECT_FALSE( configReader.hasValue( "" ) );
    EXPECT_FALSE( configReader.hasValue( "/" ) );

    EXPECT_FALSE( configReader.hasValue( "source3/" ) );
}

TEST_F( ConfigReaderTestSuite, source ) {
    bool isFirstSourceReaded = false;
    bool isSecondSourceReaded = false;
    bool isInvalidSourceReaded = false;

    const auto configReader = createConfigReader( [&]( const std::string& source ) {
        if( source == "firstSource" ) {
            EXPECT_FALSE( isFirstSourceReaded );
            isFirstSourceReaded = true;

            return JSON( "root" << JSON( "child" << 1 ) ).dump();
        }

        if( source == "secondSource" ) {
            EXPECT_FALSE( isSecondSourceReaded );
            isSecondSourceReaded = true;

            return JSON( "root" << JSON( "child" << 2 ) ).dump();
        }

        if( source == "invalidSource" ) {
            EXPECT_FALSE( isInvalidSourceReaded );
            isInvalidSourceReaded = true;
            return std::string( "{\"key\":\"1}" );
        }

        return std::string();
    } );

    EXPECT_EQ( configReader.getInt( "firstSource/root/child" ), 1 );
    EXPECT_EQ( configReader.getInt( "secondSource/root/child" ), 2 );

    // do twice
    EXPECT_EQ( configReader.getInt( "firstSource/root/child" ), 1 );
    EXPECT_EQ( configReader.getInt( "secondSource/root/child" ), 2 );

    EXPECT_EQ( configReader.getInt( "/secondSource/root/child" ), 2 );

    EXPECT_THROW( configReader.getInt( "firstSource//root/child" ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getInt( "firstSourceroot/child" ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getInt( "//secondSource/root/child" ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getInt( "firstSourc/eroot/child" ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getInt( "secondSource" ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getInt( "/root/child" ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getInt( "invalidSource/key" ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getString( "invalidSource/key" ), ConfigReader::ReadingError );
}

TEST_F( ConfigReaderTestSuite, configReaderException ) {
    const auto configReader =
        createConfigReader( []( const std::string& ) -> std::string { throw std::logic_error( "exception" ); } );

    EXPECT_THROW( configReader.getString( "source/key" ), ConfigReader::ReadingError );
}

TEST_F( ConfigReaderTestSuite, threadSafe ) {
    static constexpr size_t ThreadsCount = 200u;
    static constexpr size_t SourcesCount = 50u;

    std::set< std::string > initializedSources;

    const auto configReader = createConfigReader( [&]( const std::string& source ) {
        const auto insertResult = initializedSources.insert( source );
        EXPECT_TRUE( insertResult.second );

        return JSON( "key" + source << source ).dump();
    } );

    std::vector< std::thread > threads;

    std::vector< std::string > sources;
    for( size_t sourceIndex = 0; sourceIndex < SourcesCount; ++sourceIndex ) {
        sources.emplace_back( std::to_string( sourceIndex ) );
    }

    for( size_t threadIndex = 0; threadIndex < ThreadsCount; ++threadIndex ) {
        threads.emplace_back( [&]() {
            for( const auto& source : sources ) {
                const auto path = source + "/key" + source;
                EXPECT_EQ( configReader.getString( path.c_str() ), source );
            }
        } );
    }

    for( auto& thread : threads ) {
        thread.join();
    }

    EXPECT_EQ( initializedSources.size(), SourcesCount );

    for( const auto& source : sources ) {
        for( const auto& keySuffix : sources ) {
            if( source != keySuffix ) {
                const auto path = source + "/key" + keySuffix;
                EXPECT_THROW( configReader.getString( path.c_str() ), ConfigReader::ReadingError );
            }
        }
    }
}

TEST_F( ConfigReaderTestSuite, configReaderPath ) {
    const auto configReader = createConfigReader( []( const std::string& ) {
        return JSON( "root" << JSON( "array" << JSON_ARRAY( 9 << "text" << true
                                                              << JSON( "stringField"
                                                                       << "txt"
                                                                       << "intField" << 10 ) ) ) )
            .dump();
    } );

    {
        ConfigReader::Path path( configReader );

        path = std::move( path ) / "source/root/array";
        EXPECT_EQ( ConfigReader::getArraySize( path ), 4 );

        EXPECT_EQ( ConfigReader::getInt( path / size_t( 0 ) ), 9 );
        EXPECT_EQ( ConfigReader::getString( path / 1 ), "text" );
        EXPECT_EQ( ConfigReader::getBool( path / 2 ), true );

        EXPECT_EQ( ConfigReader::getString( path / 3 / "stringField" ), "txt" );
        EXPECT_EQ( ConfigReader::getInt( path / 3 / "intField" ), 10 );
    }

    {
        const ConfigReader::Path path( configReader, "source/root/array" );
        EXPECT_EQ( ConfigReader::getArraySize( path ), 4 );

        EXPECT_EQ( ConfigReader::getString( path / 3 / "stringField" ), "txt" );
    }

    {
        const ConfigReader::Path path( configReader, "/source/root/array" );
        EXPECT_EQ( ConfigReader::getArraySize( path ), 4 );

        EXPECT_EQ( ConfigReader::getString( path / 3 / "stringField" ), "txt" );
    }
}

TEST_F( ConfigReaderTestSuite, configReaderPathWithDeletedOwner ) {
    auto configReader = std::make_shared< ConfigReader >(
        createConfigReader( []( const std::string& ) { return JSON( "root" << JSON( "field" << 8 ) ).dump(); } ) );

    auto path = std::make_shared< ConfigReader::Path >( *configReader, "source/root/field" );

    configReader.reset();
    EXPECT_EQ( ConfigReader::getInt( *path ), 8 );
    path.reset();
}

TEST_F( ConfigReaderTestSuite, configReaderInvalidPath ) {
    const auto configReader = createConfigReader( []( const std::string& source ) {
        if( source == "source" ) {
            return JSON( "root" << JSON( "array" << JSON_ARRAY( 1 << JSON( "stringField"
                                                                           << "txt"
                                                                           << "intField" << 10 ) ) ) )
                .dump();
        }

        return std::string();
    } );

    EXPECT_THROW( ConfigReader::Path( configReader, "sourc" ), ConfigReader::ReadingError );
    EXPECT_THROW( ConfigReader::Path( configReader, "//source" ), ConfigReader::ReadingError );
    EXPECT_THROW( ConfigReader::Path( configReader, "source/" ), ConfigReader::ReadingError );
    EXPECT_THROW( ConfigReader::Path( configReader, "/" ), ConfigReader::ReadingError );
    EXPECT_THROW( ConfigReader::Path( configReader, "source//array" ), ConfigReader::ReadingError );
    EXPECT_THROW( ConfigReader::Path( configReader, "/source/array" ), ConfigReader::ReadingError );

    ConfigReader::Path path( configReader, "source" );
    EXPECT_THROW( path / "arr", ConfigReader::ReadingError );
    EXPECT_THROW( path / size_t( 0 ), ConfigReader::ReadingError );
    EXPECT_THROW( path / "", ConfigReader::ReadingError );

    path = std::move( path ) / "root/array";

    EXPECT_THROW( path / 2, ConfigReader::ReadingError );
    EXPECT_THROW( path / "0", ConfigReader::ReadingError );

    path = std::move( path ) / 1;

    EXPECT_THROW( path / size_t( 0 ), ConfigReader::ReadingError );
    EXPECT_THROW( path / "txt", ConfigReader::ReadingError );

    path = ConfigReader::Path( configReader );
    EXPECT_THROW( ConfigReader::getInt( path ), ConfigReader::ReadingError );
    EXPECT_THROW( ConfigReader::getBool( path ), ConfigReader::ReadingError );
    EXPECT_THROW( ConfigReader::getString( path ), ConfigReader::ReadingError );
    EXPECT_THROW( ConfigReader::getFloat( path ), ConfigReader::ReadingError );
    EXPECT_THROW( ConfigReader::getArraySize( path ), ConfigReader::ReadingError );
    EXPECT_THROW( path / size_t( 0 ), ConfigReader::ReadingError );
}

TEST_F( ConfigReaderTestSuite, configReaderSpecialSources ) {
    const auto configReader = createConfigReaderWithSpecialSources(
        []( const std::string& source ) {
            return JSON( "root" << JSON( "field" << 8 << "field2"
                                                 << "txt" ) )
                .dump();
        },
        { { "provision",
            []( const std::string& source ) { return JSON( "root" << JSON( "field" << 10 ) ).dump(); } } } );

    EXPECT_EQ( configReader.getInt( "source/root/field" ), 8 );
    EXPECT_EQ( configReader.getString( "source/root/field2" ), "txt" );
    EXPECT_EQ( configReader.getInt( "provision/root/field" ), 10 );
    EXPECT_THROW( configReader.getInt( "provision/root/field2" ), ConfigReader::ReadingError );
}

TEST_F( ConfigReaderTestSuite, valueWithSingleVariable ) {
    const auto configReader =
        createConfigReader( []( const std::string& source ) {
            if( source == "source" ) {
                return JSON( "variableField"
                             << "${firmware_root}" )
                    .dump();
            }

            return std::string();
        } );

    EXPECT_FALSE( Environment::getFirmwareRootFolder().empty() ) << "Firmware root folder should be filled!";
    EXPECT_TRUE( configReader.hasValue( "source/variableField" ) );
    EXPECT_EQ( configReader.getString( "source/variableField" ), Environment::getFirmwareRootFolder() )
        << "${firmware_root} variable should be replaced by Environment::getFirmwareRootFolder() value.";
}

TEST_F( ConfigReaderTestSuite, valueWithTwoVariables ) {
    const auto configReader =
        createConfigReader( []( const std::string& source ) {
            if( source == "source" ) {
                return JSON( "variableField"
                             << "${firmware_root}:${firmware_bin}" )
                    .dump();
            }

            return std::string();
        } );

    EXPECT_FALSE( Environment::getFirmwareRootFolder().empty() ) << "Firmware root folder should be filled!";
    EXPECT_FALSE( Environment::getBinariesFolder().empty() ) << "Firmware binaries folder should be filled!";

    EXPECT_TRUE( configReader.hasValue( "source/variableField" ) );
    EXPECT_EQ( configReader.getString( "source/variableField" ),
               Environment::getFirmwareRootFolder() + ":" + Environment::getBinariesFolder() );
}

TEST_F( ConfigReaderTestSuite, valueWithThreeVariables ) {
    const auto configReader =
        createConfigReader( []( const std::string& source ) {
            if( source == "source" ) {
                return JSON( "variableField"
                             << "${firmware_root}/${firmware_bin}/${firmware_etc}" )
                    .dump();
            }

            return std::string();
        } );

    EXPECT_FALSE( Environment::getFirmwareRootFolder().empty() ) << "Firmware root folder should be filled!";
    EXPECT_FALSE( Environment::getBinariesFolder().empty() ) << "Firmware binaries folder should be filled!";
    EXPECT_FALSE( Environment::getConfigsFolder().empty() ) << "Firmware config folder should be filled!";

    EXPECT_TRUE( configReader.hasValue( "source/variableField" ) );
    EXPECT_EQ( configReader.getString( "source/variableField" ),
               Environment::getFirmwareRootFolder() + "/" + Environment::getBinariesFolder() + "/" +
                   Environment::getConfigsFolder() );
}

TEST_F( ConfigReaderTestSuite, keyWithSingleVariable ) {
    const auto configReader = createConfigReader( []( const std::string& source ) {
        if( source == "source" ) {
            return JSON( "${firmware_root}"
                         << "Simple string" )
                .dump();
        }

        return std::string();
    } );

    EXPECT_TRUE( configReader.hasValue( "source/${firmware_root}" ) ) << "The key shouldn't be replaced!";
    EXPECT_EQ( configReader.getString( "source/${firmware_root}" ), "Simple string" )
        << "The key shouldn't be replaced!";
}

TEST_F( ConfigReaderTestSuite, valueWithNotClosedVariable ) {
    const auto configReader =
        createConfigReader( []( const std::string& source ) {
            if( source == "source" ) {
                return JSON( "variableField"
                             << "${firmware_root" )
                    .dump();
            }

            return std::string();
        } );

    EXPECT_TRUE( configReader.hasValue( "source/variableField" ) );
    EXPECT_EQ( configReader.getString( "source/variableField" ), "${firmware_root" )
        << "Not closed variable shouldn't be replaced!";
}

TEST_F( ConfigReaderTestSuite, valueWithNotOpenedVariable ) {
    const auto configReader =
        createConfigReader( []( const std::string& source ) {
            if( source == "source" ) {
                return JSON( "variableField"
                             << "firmware_root}" )
                    .dump();
            }

            return std::string();
        } );

    EXPECT_TRUE( configReader.hasValue( "source/variableField" ) );
    EXPECT_EQ( configReader.getString( "source/variableField" ), "firmware_root}" )
        << "Not opened variable shouldn't be replaced!";
}

TEST_F( ConfigReaderTestSuite, valueWithNotExistVariable ) {
    const auto configReader =
        createConfigReader( []( const std::string& source ) {
            if( source == "source" ) {
                return JSON( "variableField"
                             << "${not_exist}" )
                    .dump();
            }

            return std::string();
        } );

    EXPECT_TRUE( configReader.hasValue( "source/variableField" ) );
    EXPECT_EQ( configReader.getString( "source/variableField" ), "${not_exist}" )
        << "If a variable is not exist, It should be not changed!";
}

TEST_F( ConfigReaderTestSuite, valueWithVariableBetweenSimpleText ) {
    const auto configReader =
        createConfigReader( []( const std::string& source ) {
            if( source == "source" ) {
                return JSON( "variableField"
                             << "Start${firmware_root}End" )
                    .dump();
            }

            return std::string();
        } );

    EXPECT_FALSE( Environment::getFirmwareRootFolder().empty() ) << "Firmware root folder should be filled!";
    EXPECT_TRUE( configReader.hasValue( "source/variableField" ) );
    EXPECT_EQ( configReader.getString( "source/variableField" ),
               "Start" + Environment::getFirmwareRootFolder() + "End" );
}

TEST_F( ConfigReaderTestSuite, getObjectKeys ) {
    const auto configReader = createConfigReader( []( const std::string& source ) {
        if( source == "source1" ) {
            return JSON( "obj" << 2 ).dump();
        }

        if( source == "source2" ) {
            return JSON( "obj" << JSON_EMPTY_OBJ ).dump();
        }

        if( source == "source3" ) {
            return JSON( "obj" << JSON( "1" << 1 << "2" << 2 ) ).dump();
        }

        return std::string();
    } );

    EXPECT_THROW( configReader.getObjectKeys( ConfigReader::Path( configReader, "source1/obj" ) ),
                  ConfigReader::ReadingError );
    EXPECT_EQ( configReader.getObjectKeys( ConfigReader::Path( configReader, "source2/obj" ) ).size(), 0 );
    EXPECT_EQ( configReader.getObjectKeys( ConfigReader::Path( configReader, "source3/obj" ) ).size(), 2 );
}

TEST_F( ConfigReaderTestSuite, jsonArrayField ) {
    const char fieldName[] = "source/jsonArrayField";
    const size_t arraySize = 5;
    const auto getJsonValue = []( size_t i ) -> JsonObject { return JSON( "feature" << i << "service" << i ); };
    const auto configReader = createConfigReader( [&getJsonValue, &fieldName]( const JsonObject& ) {
        JsonArray jArray;
        for( size_t i = 0; i < arraySize; ++i ) {
            jArray.push_back( getJsonValue( i ) );
        }
        return JSON( "jsonArrayField" << jArray ).dump();
    } );
    const auto path = ConfigReader::Path( configReader, fieldName );
    EXPECT_EQ( configReader.getArraySize( path ), arraySize );

    const auto jsonVec = configReader.getJsonArray( path );
    for( size_t i = 0; i < arraySize; ++i ) {
        EXPECT_EQ( jsonVec[ i ], JSON( "feature" << i << "service" << i ) );
    }

    EXPECT_THROW( configReader.getJsonArray( ConfigReader::Path( configReader, "source/invalidField" ) ),
                  ConfigReader::ReadingError );

    EXPECT_THROW( configReader.getInt( fieldName ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getFloat( fieldName ), ConfigReader::ReadingError );
    EXPECT_THROW( configReader.getBool( fieldName ), ConfigReader::ReadingError );
}

}  // namespace UT
}  // namespace HubPlatform
