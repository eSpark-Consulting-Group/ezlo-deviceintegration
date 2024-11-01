#ifndef ANSWERS_H
#define ANSWERS_H

#include "json_utils/json_wrapper.h"
#include "jsonrpc/legacy_synchronization_traits.h"

#include <tiny/tiny_ws.h>
#include <tiny/tiny_atomic_shared_ptr.h>
#include <tiny/tiny_multithreading.h>

#include <ctime>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>

class Answers {
public:
    using GenericSender = tiny::ws::generic_sender;
    using network_object = tiny::retain_release_ptr< GenericSender, rpc::LegacySynchronizationTraits< GenericSender > >;
    static void add( network_object pNetwork, JsonObject& joAnswer, const std::string& strMethod );
    static bool remove( const JsonObject& joMessage );

private:
    Answers();
    ~Answers();
    static Answers& instance();

    void timeoutThread();
    void checkAndSend();
    void sendByTimeout( const std::string& operationId );

    typedef tiny::multithreading::recursive_mutex_type mutex_type;
    typedef std::lock_guard< mutex_type > lock_type;
    mutex_type mtx_;

    typedef std::pair< JsonObject, network_object > receiver_type;
    typedef std::map< std::string, receiver_type > answer_queue;
    answer_queue answersQueue_;

    typedef std::multimap< time_t, std::string > time_list;
    time_list timeList_;

    std::atomic_bool stopThreadFlag_;
    std::thread thread_;
};  /// class Answers

#endif  // ANSWERS_H
