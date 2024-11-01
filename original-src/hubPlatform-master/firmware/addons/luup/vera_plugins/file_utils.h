#pragma once
#include "xml_wrapper.h"
#include <string>

namespace FileUtils {
void makeDir( std::string directory );
std::string getDir( const std::string& absoluteFileName );
std::string readFile( const std::string& fileName );
bool dirExists( const std::string& dirName );
void decryptFile( std::string& inOutData );
XmlDocument parseXmlFile( const std::string& fileName );
};  // namespace FileUtils
