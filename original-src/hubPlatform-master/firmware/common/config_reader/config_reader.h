#pragma once

#include <map>
#include <set>
#include <memory>
#include <string>
#include <functional>
#include <vector>
#include "json_utils/json_fwd.h"

namespace HubPlatform {

namespace UT {
class ConfigReaderFactory;
}  // namespace UT

/* @brief Provides configuration reading from files in directory CONFIG_DIR
 *
 * @note Reads values by path in format source/group1/group2/key where
 *       CONFIG_DIR/source.cfg is config file in json format, group1/group2/../groupN/key is path to value in json.
 */
class ConfigReader {
    struct PrivateTag {};

public:
    class ReadingError : public std::logic_error {
    public:
        using logic_error::logic_error;
    };

    class Path {
        class Impl;
        using ImplUPtr = std::unique_ptr< Impl >;

    public:
        explicit Path( const ConfigReader& configReader, const char* path = "" );

        Path( Path&& path ) noexcept;
        Path& operator=( Path&& path ) noexcept;
        Path( const Path& path ) noexcept;
        Path& operator=( const Path& path ) noexcept;

        ~Path();

        Path operator/( const char* subPath ) const&;
        Path operator/( const char* subPath ) &&;

        Path operator/( size_t index ) const&;
        Path operator/( size_t index ) &&;

        const Impl& getImpl( PrivateTag ) const noexcept;

    private:
        ImplUPtr m_impl;
    };

    ConfigReader();

    ConfigReader( const ConfigReader& ) = delete;
    ConfigReader& operator=( const ConfigReader& ) = delete;

    ConfigReader( ConfigReader&& ) noexcept = default;
    ConfigReader& operator=( ConfigReader&& ) noexcept = default;

    /* @brief get integer value from config
     *
     * @throws ReadingError if path is invalid or actual type is not int/float
     * @returns int or casted float value from config
     */
    int getInt( const char* path ) const;
    static int getInt( const Path& path );

    /* @brief get float value from config
     *
     * @throws ReadingError if path is invalid or actual type is not int/float
     * @returns float or casted int value from config
     */
    float getFloat( const char* path ) const;
    static float getFloat( const Path& path );

    /* @brief get bool value from config
     *
     * @throws ReadingError if path is invalid or actual type is not bool
     * @returns bool value from config
     */
    bool getBool( const char* path ) const;
    static bool getBool( const Path& path );

    /* @brief get string value from config
     *
     * @throws ReadingError if path is invalid or actual type is not string
     * @returns const std::string& value from config
     */
    const std::string& getString( const char* path ) const;
    static const std::string& getString( const Path& path );

    /* @brief checks is bool, int, float or string option exists
     */
    bool hasValue( const char* path ) const noexcept;
    static bool hasValue( const Path& path ) noexcept;

    /* @brief get array size from config
     *
     * @throws ReadingError if path is invalid or actual type is not array
     * @returns size_t array size
     */
    static size_t getArraySize( const Path& path );

    /* @brief get vector of string from config
     *
     * @throws ReadingError if path is invalid or actual type is not string
     * @returns std::vector< std::string > value from config
     */
    static std::vector< std::string > getStringArray( const Path& path );

    /* @brief get vector of json from config
     *
     * @throws ReadingError if path is invalid or actual type is not json
     * @returns std::vector< json > value from config
     */
    static std::vector< JsonObject > getJsonArray( const Path& path );

    /* @brief get object's keys
     *
     * @throws ReadingError if path is invalid or actual type is not object
     * @returns object's keys as an std::vector< std::string > value
     */
    static std::vector< std::string > getObjectKeys( const Path& path );

    /* @brief get object
     *
     * @throws ReadingError if path is invalid or actual type is not object
     * @returns object as a JsonObject value
     */
    static JsonObject getJsonObject( const Path& path );

private:
    using StreamUPtr = std::unique_ptr< std::istream >;
    using SourceReader = std::function< StreamUPtr( const std::string& ) >;
    using SpecialSourceReaders = std::map< std::string, SourceReader >;

    ConfigReader( SourceReader sourceReader, SpecialSourceReaders specialSourceReaders );

private:
    class Impl;
    using ImplPtr = std::shared_ptr< Impl >;
    ImplPtr m_impl;

    friend class UT::ConfigReaderFactory;
};

}  // namespace HubPlatform
