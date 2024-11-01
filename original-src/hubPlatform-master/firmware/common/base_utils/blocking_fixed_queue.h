#include "base_utils/blocking_queue.h"
#include "log_utils/ezlo_log.h"
#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace Container {

enum class QueueSize { DEFAULT_QUEUE_LIMIT = 150 };

template < typename T >
class BlockingFixedQueue : public BlockingQueue< T > {
public:
    BlockingFixedQueue()
    : m_queueLimit( static_cast< size_t >( QueueSize::DEFAULT_QUEUE_LIMIT ) ) {}

    BlockingFixedQueue( size_t qSize )
    : m_queueLimit( qSize ) {
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
    void push_iml( const T& value ) override {
        std::unique_lock< std::mutex > lock( this->m_queueMutex );
        while( this->m_queue.size() >= m_queueLimit ) {
            this->m_queue.pop();
        }
        this->m_queue.emplace( value );
        this->m_condition.notify_one();
    }

private:
    size_t m_queueLimit;
};

}  // namespace Container
}  // namespace HubPlatform
