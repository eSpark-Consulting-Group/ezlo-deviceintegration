#pragma once

#include <deque>
#include <string>
#include <memory>
#include <mutex>

namespace cc {

class IRpcMessageFilters {
public:
    virtual ~IRpcMessageFilters() = default;

    virtual size_t sizeLimit() const = 0;
    virtual void sizeLimit( size_t limit ) = 0;
    virtual bool needFiltering( const std::string& data ) const = 0;
    virtual bool updateJsonDump( const std::string& data, std::string& truncated ) = 0;
};

class RpcMessageFiltersBySize : public IRpcMessageFilters {
public:
    RpcMessageFiltersBySize();
    enum class Limits : size_t { SIZE_LIMIT = 2048 };
    size_t sizeLimit() const override;
    void sizeLimit( size_t limit ) override {
        m_limit = limit;
    };

    bool needFiltering( const std::string& data ) const override;
    /**
     *  apply filters on 'data'
     *  if a pattern is found, convert string to json.
     *       apply truncation actions
     *  else
     *       just truncate raw data
     */
    bool updateJsonDump( const std::string& data, std::string& truncated ) override;

private:
    size_t m_limit;
};

/** Singleton - register message filters with this singleton to truncate messages coming in and out of the
 * websockets client.
 */
class RpcMessageFiltersSingleton final {
public:
    RpcMessageFiltersSingleton( const RpcMessageFiltersSingleton& ) = delete;
    RpcMessageFiltersSingleton& operator=( const RpcMessageFiltersSingleton& ) = delete;

    static RpcMessageFiltersSingleton& instance();

    using FiltersPtr = std::shared_ptr< IRpcMessageFilters >;

    void pushBack( std::shared_ptr< IRpcMessageFilters > filters );
    void pushFront( std::shared_ptr< IRpcMessageFilters > filters );
    void reset();

    bool processFilters( const std::string& data, std::string& filtered ) const;

private:
    RpcMessageFiltersSingleton() = default;

private:
    using Mutex = std::mutex;
    using Locker = std::unique_lock< Mutex >;

private:
    mutable Mutex m_mutex;
    std::deque< FiltersPtr > m_filters;
};

}  // namespace cc