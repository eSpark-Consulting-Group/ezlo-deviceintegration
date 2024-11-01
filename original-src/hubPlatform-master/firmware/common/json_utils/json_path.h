#pragma once
#include "json_utils/json_fwd.h"
#include <string>
#include <vector>
#include <functional>

class JsonPath {
public:
    using Keys = std::vector< Json >;
    JsonPath() = default;
    JsonPath( const JsonPath& ) = default;
    JsonPath& operator=( const JsonPath& ) = default;
    JsonPath( JsonPath&& ) = default;
    JsonPath& operator=( JsonPath&& ) = default;
    JsonPath( std::initializer_list< Json > init );
    explicit JsonPath( Keys init );

public:
    void pushBack( Json key );
    void popBack();
    Json back();
    size_t size() const;
    bool empty() const;
    std::string toString() const;
    const Keys& keys() const;
    void append( const JsonPath& add );

public:
    class Guard {
    public:
        Guard( Guard&& r ) noexcept;
        Guard( JsonPath& path, Json key );
        ~Guard();

    private:
        std::reference_wrapper< JsonPath > m_path;
    };

    JsonPath::Guard makeGuard( Json key );

private:
    std::vector< Json > m_keys;
};