
#include "database_client/lmdb_client.h"
#include "database_client/dbqueryparser.h"
#include "database_client/lmdb++.h"
#include "database_client/dbtools.h"

enum class Action : int { NO_ACTION, PRINT_LIST, PRINT_STORAGE_PATH, PRINT_HELP, PRINT_TABLE_KEYS };

JsonObject parseJson( const std::string& jsonStr ) {
    JsonObject json = JSON_EMPTY_OBJ;
    if( !jsonStr.empty() ) {
        try {
            json = Json::parse( jsonStr );
        }
        catch( ... ) {
            std::cerr << "Incorrect json!" << std::endl;
        }
    }
    return json;
}

void printHelp() {
    std::cerr << "Wrong params count" << std::endl;
    std::cout << "dbreader -h. Print help" << std::endl;
    std::cout << "dbreader -s. Set database path. If not set then default path is used" << std::endl;
    std::cout << "dbreader -p. Print database path" << std::endl;
    std::cout << "dbreader -l. List of tables which database consist" << std::endl;
    std::cout << "dbreader -t table_name. List of rows which table consist" << std::endl;
    std::cout << "dbreader -t table_name -id id_value. All notes from table which include this id" << std::endl;
    std::cout << "dbreader -id id_value. All notes from database which include this id" << std::endl;
    std::cout << "dbreader -j json. All notes from database which include this json" << std::endl;
    std::cout << "dbreader -t table_name -j json. Add record to table" << std::endl;
    std::cout << "dbreader -t table_name -pk. Print primary keys of table" << std::endl;
    std::cout << "dbreader -t table_name -r json. Remove record from table" << std::endl;
    std::cout << "dbreader -t table_name -rid id. Remove record with current id from table" << std::endl;
    std::cout << "dbreader -rt table_name. Remove table" << std::endl;
}

void printAllTables( const std::vector< std::string >& tables, const std::string& tableListName ) {
    std::cout << " " << tableListName << ": { " << std::endl;
    for( const auto& table : tables ) {
        std::cout << " \t" << table << std::endl;
    }
    std::cout << " }" << std::endl;
}

void printAllRecords( const IDbClient& client,
                      const std::vector< std::string >& tables,
                      const JsonObject& query,
                      int indentSpacesAmount ) {
    for( const auto& table : tables ) {
        for( JsonObject& element : client.findAll( table, query, nullptr, 1, 0, false ) )
            std::cout << element.dump( indentSpacesAmount ) << std::endl;
    }
}

int main( int argc, char* argv[] ) {
    std::string dbPath;

    if( argc < 2 ) {
        printHelp();
        return 0;
    }

    dbPath = getStorageFilePath();

    if( dbPath.empty() ) {
        std::cout << "Please. input path fo data base" << std::endl;
        return -1;
    }
    try {
        Action action = Action::NO_ACTION;
        std::string tableName, id, jsonStr, removeRecord, idToRemove, removeTable;

        for( auto i = 0; i < argc; ++i ) {
            if( strcmp( argv[ i ], "-h" ) == 0 ) {
                action = Action::PRINT_HELP;
            }
            else if( strcmp( argv[ i ], "-l" ) == 0 ) {
                action = Action::PRINT_LIST;
            }
            else if( strcmp( argv[ i ], "-p" ) == 0 ) {
                action = Action::PRINT_STORAGE_PATH;
            }
            else if( strcmp( argv[ i ], "-s" ) == 0 ) {
                dbPath = argv[ i + 1 ];
                i++;
            }
            else if( strcmp( argv[ i ], "-t" ) == 0 ) {
                tableName = argv[ i + 1 ];
                i++;
            }
            else if( strcmp( argv[ i ], "-pk" ) == 0 ) {
                action = Action::PRINT_TABLE_KEYS;
            }
            else if( strcmp( argv[ i ], "-id" ) == 0 ) {
                id = argv[ i + 1 ];
                i++;
            }
            else if( strcmp( argv[ i ], "-j" ) == 0 ) {
                jsonStr = argv[ i + 1 ];
                i++;
            }
            else if( strcmp( argv[ i ], "-r" ) == 0 ) {
                removeRecord = argv[ i + 1 ];
                i++;
            }
            else if( strcmp( argv[ i ], "-rid" ) == 0 ) {
                idToRemove = argv[ i + 1 ];
                i++;
            }
            else if( strcmp( argv[ i ], "-rt" ) == 0 ) {
                removeTable = argv[ i + 1 ];
                i++;
            }
        }

        const int indentSpacesAmount = 4;
        LMDBClient client( dbPath, getTemporaryStorageFilePath() );

        if( action == Action::PRINT_HELP ) {
            printHelp();
        }
        else if( action == Action::PRINT_LIST ) {
            printAllTables( client.getPermanentCollectionsNames(), "permanent tables" );
            printAllTables( client.getTemporaryCollectionsNames(), "temporary tables" );
        }
        else if( action == Action::PRINT_STORAGE_PATH ) {
            std::cout << "permanent database path: " << dbPath << std::endl;
            std::cout << "temporary database path: " << getTemporaryStorageFilePath() << std::endl;
        }
        else if( action == Action::PRINT_TABLE_KEYS ) {
            for( const auto& key : client.getPrimaryKeys( tableName ) ) {
                std::cout << key << std::endl;
            }
        }
        else if( !tableName.empty() && !id.empty() ) {
            for( JsonObject& element : client.findAll( tableName, JSON( "_id" << id ), nullptr, 1, 0, false ) ) {
                std::cout << element.dump( indentSpacesAmount ) << std::endl;
            }
        }
        else if( !tableName.empty() && !jsonStr.empty() ) {
            client.insert( tableName, parseJson( jsonStr ) );
        }
        else if( !tableName.empty() && !removeRecord.empty() ) {
            if( removeRecord == "all" ) {
                client.remove( tableName, JSON_EMPTY_OBJ, false, false );
            }
            auto json = parseJson( removeRecord );
            if( !json.empty() ) {
                client.remove( tableName, json, false, false );
            }
        }
        else if( !tableName.empty() && !idToRemove.empty() ) {
            JsonObject json = JSON_EMPTY_OBJ;
            json[ "_id" ] = idToRemove;
            client.remove( tableName, json, false, false );
        }
        else if( !tableName.empty() ) {
            auto content = client.dumpCollection( tableName, indentSpacesAmount );
            std::cout << content << std::endl;
        }
        else if( !jsonStr.empty() ) {
            auto json = parseJson( jsonStr );
            if( !json.empty() ) {
                printAllRecords( client, client.getPermanentCollectionsNames(), json, indentSpacesAmount );
                printAllRecords( client, client.getTemporaryCollectionsNames(), json, indentSpacesAmount );
            }
        }
        else if( !id.empty() ) {
            printAllRecords( client, client.getPermanentCollectionsNames(), JSON( "_id" << id ), indentSpacesAmount );
            printAllRecords( client, client.getTemporaryCollectionsNames(), JSON( "_id" << id ), indentSpacesAmount );
        }
        else if( !removeTable.empty() ) {
            client.drop( removeTable );
        }
        else {
            std::cout << "Nothing done!" << std::endl;
        }
    }
    catch( const std::runtime_error& error ) {
        std::cerr << error.what() << std::endl;
        return -1;
    }

    return EXIT_SUCCESS;
}
