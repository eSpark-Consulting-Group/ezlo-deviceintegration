#include "extensions_manager/plugin_model/plugin_package_validator.h"
#include "extensions_manager/plugin_model/plugins_metedata_validator.h"

#include "json_utils/json_wrapper.h"

#include <fstream>
#include <iostream>
#include <string>
#include <stdexcept>

using namespace HubPlatform::ExtensionsManager;

namespace {
class InvalidArgs : public std::invalid_argument {
public:
    using invalid_argument::invalid_argument;
};

void printError( const std::string& message = {} ) {
    std::string msg = !message.empty() ? message : "empty error message";
    std::cerr << msg << "\n\n";
}

void printUsage( const std::string& message = {} ) {
    std::string msg = !message.empty() ? message + "\n\n" : "";
    std::cerr << msg
              << "This binary is used to check a plugin package.\n"
                 "Usage:\n"
                 "  ./plugin_package_checker path_to_package.tgz [path_to_file_where_check_results_will_be_put]\n";
}

void printCheckResults( const std::string& resultsFilePath, const Json& results ) {
    if( !resultsFilePath.empty() ) {
        std::ofstream resultsFile( resultsFilePath );
        resultsFile << results.dump();
    }
}

Json makeJsonResult( const std::string& level, const std::string& message ) {
    return JSON( "error" << JSON( "level" << level << "msg" << message ) );
}

Json makeJsonResult() {
    return JSON( "error" << JSON_EMPTY_OBJ );
}
}  // namespace

int main( int argc, char* argv[] ) {
    std::string resultFile;
    Json error;

    try {
        if( argc < 2 ) {
            throw InvalidArgs( "Insufficient amount of arguments, should be at least 1" );
        }

        std::string packageFile( argv[ 1 ] );
        if( argc > 2 ) {
            resultFile = argv[ 2 ];
        }

        PluginPackageValidator pluginPackageValidator( packageFile, "" );
        pluginPackageValidator.validate();

        printCheckResults( resultFile, makeJsonResult() );
        return EXIT_SUCCESS;
    }
    catch( const ::ExtensionsManager::PluginsMetadataValidatorException& ex ) {
        auto errorMessage = ex.getPath().toString() + ": " + ex.what();
        printUsage( errorMessage );
        error = makeJsonResult( "invalid_metadata", errorMessage );
    }
    catch( const PluginPackageValidatorError& ex ) {
        printError( ex.what() );
        error = makeJsonResult( "invalid_package", ex.what() );
    }
    catch( const InvalidArgs& ex ) {
        printUsage( ex.what() );
        error = makeJsonResult( "app", ex.what() );
    }
    catch( const std::exception& ex ) {
        printError( ex.what() );
    }
    catch( ... ) {
        printError( "Unknown internal error occurred" );
    }

    printCheckResults( resultFile, error );
    return EXIT_FAILURE;
}
