#pragma once

#include "uci_utils/uci_file.h"

namespace HubPlatform {
namespace UciUtils {

class UciDhcpFile final : public UciFile {
public:
    explicit UciDhcpFile( std::string fileName );
    UciDhcpFile();

    bool apply();

private:
};

}  // namespace UciUtils
}  // namespace HubPlatform
