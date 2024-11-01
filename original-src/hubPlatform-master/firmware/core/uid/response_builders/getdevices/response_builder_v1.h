#pragma once

#include "iresponse_builder.h"

#include "uid/response_builders/response_builder_base.h"
#include "uid/response_builders/filter_data_list.h"

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {
namespace GetDevices {
namespace V1 {

class ResponseBuilder : public ResponseBuilderBase, public IResponseBuilder {
public:
    ResponseBuilder( const rpc::Message& message, const std::string& api, FilterDataListI* filter = nullptr );

    void addDevice( const std::string& id,
                    const std::string& deviceTypeId,
                    const std::string& gatewayId,
                    const std::string& name,
                    const std::string& type,
                    const std::string& roomId,
                    const std::string& category,
                    const std::string& subcategory,
                    const std::string& parentDeviceId,
                    const std::string& security,
                    const std::string& status,
                    const std::vector< DeviceAdditionalInfo >& additionalInfo,
                    const std::vector< FirmwareInfo >& firmware,
                    bool batteryPowered,
                    bool reachable,
                    bool ready,
                    ArmedState armed,
                    bool persistent,
                    bool parentRoom,
                    const std::string& protectConfig ) override;

    JsonObject buildResponse() const override;

protected:
    std::vector< JsonObject > m_devices;
    FilterDataListI* m_filter;
};

}  // namespace V1
}  // namespace GetDevices

namespace DataListDevices {
namespace V1 {

class ResponseBuilder : virtual public GetDevices::V1::ResponseBuilder, public DataListDevices::IResponseBuilder {
public:
    ResponseBuilder( const rpc::Message& message, const std::string& api, FilterDataListI* filter );

    void addDevice( const std::string& id,
                    const std::string& deviceTypeId,
                    const std::string& gatewayId,
                    const std::string& name,
                    const std::string& type,
                    const std::string& roomId,
                    const std::string& category,
                    const std::string& subcategory,
                    const std::string& parentDeviceId,
                    const std::string& security,
                    const std::string& status,
                    const std::vector< DeviceAdditionalInfo >& additionalInfo,
                    const std::vector< FirmwareInfo >& firmware,
                    bool batteryPowered,
                    bool reachable,
                    bool ready,
                    GetDevices::ArmedState armed,
                    bool persistent,
                    bool parentRoom,
                    const std::string& protectConfig ) override;

    JsonObject buildResponse() const override;

protected:
    void fillResultWithMissedIds( JsonObject& result ) const;
};

}  // namespace V1
}  // namespace DataListDevices

}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform
