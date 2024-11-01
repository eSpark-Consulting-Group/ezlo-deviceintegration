#pragma once

#include "base_service.h"
#include "mongoose.h"
#include "c_callback_wrapper.h"

class MongooseService final : public BaseService {
public:
    MongooseService( HubPlatform::Addons::Addon& addon, const std::string& webRoot );

    ~MongooseService() override;

    void start() override;

private:
    void handleDataRequest( mg_connection* connection, const mg_request_info* ri, void* data );

    static const char* getResponseContentType( ResponseFormat responseFormat );

private:
    mg_context* m_context;

    CCallbackWrapper< decltype( &MongooseService::handleDataRequest ), &MongooseService::handleDataRequest >
        m_handleDataRequestWrapper;
};
