#include "answers.h"
#include "uid/nma_client.h"
#include "base_utils/timeutils.h"
#include <chrono>

Answers::Answers()
: mtx_( "RMTX-Answers" )
, stopThreadFlag_( false )
, thread_( &Answers::timeoutThread, this ) {}
Answers::~Answers() {
    stopThreadFlag_ = true;
    try {
        if( thread_.joinable() )
            thread_.join();
    }
    catch( ... ) {
    }
}

Answers& Answers::instance() {
    static Answers g_instance;
    return g_instance;
}

void Answers::add( Answers::network_object pNetwork, JsonObject& joAnswer, const std::string& strMethod ) {
    const std::string operationId = json_get_str( joAnswer, "id" );
    lock_type lock( instance().mtx_ );
    time_t timeout = 0;
    if( strMethod == "get_routing_table" )
        timeout = 10;
    //"update_routing_table"
    instance().timeList_.insert( { TimeUtils::getCurrentTime() + timeout, operationId } );
    instance().answersQueue_[ operationId ] = { joAnswer, pNetwork };
    joAnswer = nullptr;
}

bool Answers::remove( const JsonObject& joMessage ) {
    const JsonObject& joResult = jsonGetObject( joMessage, "result" );
    const std::string operationId = json_get_str( joResult, "operationId" );
    lock_type lock( instance().mtx_ );
    answer_queue& answersQueue = instance().answersQueue_;
    answer_queue::iterator itFind = answersQueue.find( operationId ), itEnd = answersQueue.end();
    if( itFind == itEnd )
        return false;
    receiver_type& rcv = itFind->second;
    JsonObject& joAnswer = rcv.first;
    network_object pNetwork = rcv.second;
    joAnswer[ "result" ] = joResult;
    if( pNetwork )
        pNetwork->sendMessage( joAnswer );
    answersQueue.erase( itFind );
    return true;
}

void Answers::timeoutThread() {
    while( !stopThreadFlag_ ) {
        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
        checkAndSend();
    }
}

void Answers::checkAndSend() {
    lock_type lock( mtx_ );
    time_t currentTime = TimeUtils::getCurrentTime();
    time_list::const_iterator itWalk = timeList_.cbegin(), itEnd = timeList_.cend();
    for( ; itWalk != itEnd; ) {
        if( currentTime >= itWalk->first ) {
            sendByTimeout( itWalk->second );
            timeList_.erase( itWalk );
            itWalk = timeList_.cbegin();
            continue;
        }
        else
            ++itWalk;  // break;
    }
}

void Answers::sendByTimeout( const std::string& operationId ) {
    answer_queue::iterator itFind = answersQueue_.find( operationId ), itEnd = answersQueue_.end();
    if( itFind == itEnd )
        return;
    receiver_type& rcv = itFind->second;
    JsonObject& joAnswer = rcv.first;
    network_object pNetwork = rcv.second;
    // 2do: update answer error message
    if( pNetwork )
        pNetwork->sendMessage( joAnswer );
    answersQueue_.erase( itFind );
}
