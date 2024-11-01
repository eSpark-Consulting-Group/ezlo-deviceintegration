#pragma once
#include "json_utils/json_fwd.h"
#include <mutex>
#include <string>
#include <atomic>
#include <queue>

namespace HubPlatform {

namespace CloudDevices {

class MultipleDataHandler {
public:
    MultipleDataHandler() = default;
    virtual void addRecordToQueue( const JsonObject& data ) = 0;
    virtual JsonObject fetchLatestRecordFromQueue() = 0;
    virtual void clearQueue() = 0;
    virtual bool isQueueEmpty() = 0;
};

class CloudDataHandler : public MultipleDataHandler {
public:
    void addRecordToQueue( const JsonObject& data ) override;
    JsonObject fetchLatestRecordFromQueue() override;
    void clearQueue() override;
    bool isQueueEmpty() override;

private:
    std::mutex m_cloudDataQueueMutex;
    std::queue< JsonObject > m_cloudDataQueue;
};

class MeshbotsTemplateHandler : public MultipleDataHandler {
public:
    void addRecordToQueue( const JsonObject& data ) override;
    JsonObject fetchLatestRecordFromQueue() override;
    void clearQueue() override;
    bool isQueueEmpty() override;

private:
    std::mutex m_meshbotTemplatesQueueMutex;
    std::queue< JsonObject > m_meshbotTemplatesQueue;
};

}  // namespace CloudDevices
}  // namespace HubPlatform