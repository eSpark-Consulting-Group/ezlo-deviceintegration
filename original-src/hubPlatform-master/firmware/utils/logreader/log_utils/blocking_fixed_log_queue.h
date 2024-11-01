#include "blocking_log_queue.h"

namespace HubPlatform {
namespace LogReader {
namespace Container {

enum class QueueSize { DEFAULT_QUEUE_LIMIT = 150 };

class BlockingFixedLogQueue : public BlockingLogQueue {
public:
    BlockingFixedLogQueue( const BlockingFixedLogQueue& ) = delete;
    BlockingFixedLogQueue& operator=( const BlockingFixedLogQueue& ) = delete;
    BlockingFixedLogQueue( BlockingFixedLogQueue&& ) = delete;
    BlockingFixedLogQueue& operator=( BlockingFixedLogQueue&& ) = delete;

    BlockingFixedLogQueue( const size_t consumerSize = 0 )
    : BlockingLogQueue( consumerSize )
    , m_queueLimit( static_cast< size_t >( QueueSize::DEFAULT_QUEUE_LIMIT ) ) {}

    BlockingFixedLogQueue( size_t qSize, const size_t consumerSize = 0 )
    : BlockingLogQueue( consumerSize )
    , m_queueLimit( qSize ) {
        if( m_queueLimit == 0 ) {
            m_queueLimit = static_cast< size_t >( QueueSize::DEFAULT_QUEUE_LIMIT );
            Log::warn( cc::warn(
                "Queue size must be greater than 0, qSize reassigned as :" + std::to_string( m_queueLimit ) + "!" ) );
        }
    }

    size_t getQueueLimit() const {
        return m_queueLimit;
    }

protected:
    void pushImpl( const std::string& value ) override {
        {
            UniqueLock lock( this->m_queueMutex );
            while( this->m_queue.size() >= m_queueLimit ) {
                popFront();
            }
        }
        BlockingLogQueue::pushImpl( value );
    }

private:
    size_t m_queueLimit;
};

}  // namespace Container
}  // namespace LogReader
}  // namespace HubPlatform
