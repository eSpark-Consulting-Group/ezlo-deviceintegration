#include "strand.h"

namespace HubPlatform {
namespace Services {

StrandId Strand::generateId() {
    static std::atomic< StrandId > counter{ 0 };
    return ++counter;
}

std::shared_ptr< Strand > Strand::createStrand( TaskLoop& taskLoop, StrandId id ) {
    // Avoiding protected ctor problem
    struct makeSharedEnabler : public Strand {
        makeSharedEnabler( TaskLoop& taskLoop, StrandId id )
        : Strand( taskLoop, id ) {}
    };
    return std::make_shared< makeSharedEnabler >( taskLoop, id );
}

void Strand::run() {
    m_threadId = std::this_thread::get_id();

    while( true ) {
        std::function< void() > handler;
        m_data.process( [&handler]( Data& data ) {
            assert( data.running );
            if( data.q.size() ) {
                handler = std::move( data.q.front() );
                data.q.pop();
            }
            else {
                data.running = false;
            }
        } );

        if( handler )
            handler();
        else
            break;
    }
    // Remove this strand.
    m_taskLoop.removeStrand( m_id );
}

}  // namespace Services
}  // namespace HubPlatform
