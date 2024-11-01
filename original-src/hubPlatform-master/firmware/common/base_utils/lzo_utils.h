#pragma once

#include <string>

namespace common {
namespace lzo_utils {

bool decompressFile( const std::string& filePath, std::string& outData );
bool decompressData( const std::string& data, std::string& outData );

}  // namespace lzo_utils
}  // namespace common
