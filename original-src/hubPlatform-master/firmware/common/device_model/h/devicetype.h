#pragma once

#include <string>
#include <vector>

class DeviceType {
public:
    DeviceType();
    explicit DeviceType( const std::string& deviceTypeId );
    explicit DeviceType( int genericType,
                         int basicType,
                         int specificType,
                         int manufacturerId,
                         int manufacturerProductType,
                         int manufacturerProductId );
    std::string getId() const;

public:
    int getGenericType() const;
    int getBasicType() const;
    int getSpecificType() const;
    int getManufacturerId() const;
    int getManufacturerProductType() const;
    int getManufacturerProductId() const;

protected:
    int m_genericType;
    int m_basicType;
    int m_specificType;
    int m_manufacturerId;
    int m_manufacturerProductType;
    int m_manufacturerProductId;
};
