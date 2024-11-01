#pragma once

#include "json_utils/json_fwd.h"
#include <string>

class DeviceAddingWizard final {
public:
    using Categories = std::pair< JsonObject, bool >;
    using Devices = std::pair< JsonObject, bool >;

    DeviceAddingWizard( const std::string& filePath );
    Categories getCategories() const;
    Devices getDevices( const std::string& categoryFilter = "" ) const;

private:
    std::pair< JsonArray, bool > getFileData() const;

private:
    std::string m_filePath;
};
