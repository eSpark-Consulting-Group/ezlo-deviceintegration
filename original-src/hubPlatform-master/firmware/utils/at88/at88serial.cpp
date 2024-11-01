
#include "at88/at88sc.h"

#include <string.h>

int main( int argc, char* argv[] ) {
    if( !strcmp( argv[ 1 ], "saveSerial" ) ) {
        std::string serial = std::string( argv[ 2 ] );
        if( serial.size() != 16 ) {
            std::cerr << "serail length must be 16 characters" << std::endl;
            return 2;
        }
        At88sc at88sc;
        if( !at88sc.writePassword() )
            std::cerr << "writePassword ERROR\n";
        if( at88sc.saveSerial( serial ) ) {
            //        at88sc.blowFuses();
            //        at88sc.blowFuses();
            //        at88sc.blowFuses();
            //        at88sc.blowFuses();
            //        at88sc.blowFuses();
            std::cerr << "serial successfully saved, and fuses were blown" << std::endl;
            return 0;
        }
        printf( "Unexpected saveSerial error\n" );
        return 3;
    }
    else if( !strcmp( argv[ 1 ], "saveMac" ) ) {
        std::string mac = std::string( argv[ 2 ] );
        At88sc at88sc;
        if( !at88sc.writePassword() )
            std::cerr << "writePassword ERROR\n";
        if( at88sc.saveMac( mac ) ) {
            std::cerr << "Mac successfully saved, and fuses were blown" << std::endl;
            return 0;
        }
        printf( "Unexpected saveMac error\n" );
        return 3;
    }
    else if( !strcmp( argv[ 1 ], "checkSerial" ) ) {
        At88sc at88sc;
        if( at88sc.getSerial().length() == 0 ) {
            return 1;
        }
    }
    else {
        std::cerr << "Wrong params count: " << argc << ". Usage: ./at88serial command data" << std::endl;
        std::cerr << "Command: saveSerial, saveMac" << std::endl;
        std::cerr << "serail length must be 16 characters" << std::endl;
        std::cerr << "MAC length must be 12 characters, without ':'" << std::endl;
        return 1;
    }
}