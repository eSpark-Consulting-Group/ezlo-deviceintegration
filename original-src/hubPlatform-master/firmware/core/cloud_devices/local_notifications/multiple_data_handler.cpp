#include <tiny/tiny_consolecolors.h>
#include "multiple_data_handler.h"
#include "log_utils/ezlo_log.h"

namespace HubPlatform {

namespace CloudDevices {
void CloudDataHandler::addRecordToQueue( const JsonObject& data ) {
    std::lock_guard< std::mutex > lock( m_cloudDataQueueMutex );
    m_cloudDataQueue.push( data );
}

void CloudDataHandler::clearQueue() {
    std::lock_guard< std::mutex > lock( m_cloudDataQueueMutex );
    while( !m_cloudDataQueue.empty() ) {
        auto data = m_cloudDataQueue.front();
        LOGGER_INFO_RAW( cc::info( "skipping update command for cloud data: " ) + cc::j( data ) );
        m_cloudDataQueue.pop();
    }
}

JsonObject CloudDataHandler::fetchLatestRecordFromQueue() {
    std::lock_guard< std::mutex > lock( m_cloudDataQueueMutex );
    return m_cloudDataQueue.back();
}

bool CloudDataHandler::isQueueEmpty() {
    std::lock_guard< std::mutex > lock( m_cloudDataQueueMutex );
    return m_cloudDataQueue.empty();
}

void MeshbotsTemplateHandler::addRecordToQueue( const JsonObject& data ) {
    std::lock_guard< std::mutex > lock( m_meshbotTemplatesQueueMutex );
    m_meshbotTemplatesQueue.push( data );
}

JsonObject MeshbotsTemplateHandler::fetchLatestRecordFromQueue() {
    std::lock_guard< std::mutex > lock( m_meshbotTemplatesQueueMutex );
    return m_meshbotTemplatesQueue.back();
}

void MeshbotsTemplateHandler::clearQueue() {
    std::lock_guard< std::mutex > lock( m_meshbotTemplatesQueueMutex );
    while( !m_meshbotTemplatesQueue.empty() ) {
        auto data = m_meshbotTemplatesQueue.front();
        LOGGER_INFO_RAW( cc::info( "skipping update command for meshbot template data: " ) + cc::j( data ) );
        m_meshbotTemplatesQueue.pop();
    }
}

bool MeshbotsTemplateHandler::isQueueEmpty() {
    std::lock_guard< std::mutex > lock( m_meshbotTemplatesQueueMutex );
    return m_meshbotTemplatesQueue.empty();
}
}  // namespace CloudDevices
}  // namespace HubPlatform