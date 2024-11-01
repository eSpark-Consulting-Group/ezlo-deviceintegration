#ifndef FIRMWARE_H
#define FIRMWARE_H

#include "bus/messagesclasses.h"
#include "json_utils/json_fwd.h"

#include <string>

class Environment;
using EnvironmentWPtr = std::weak_ptr< Environment >;

namespace firmware {
auto downloadPath() -> std::string;
void setFileDownloadProgress( std::size_t position, std::size_t fileSize );
void setFileDownloadFinished();
auto writeFilePiece( const std::string& file_name, std::size_t position, const std::string& data ) -> std::size_t;
}  // namespace firmware

#endif  // FIRMWARE_H
