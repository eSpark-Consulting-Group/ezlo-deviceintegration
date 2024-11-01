#pragma once

#include "bluetooth/exceptions/bad_property.h"
#include "bluetooth/exceptions/bad_property_type.h"
#include "bluetooth/utils/cpp_type_to_dbus_type.h"

#include <gdbus.h>
#include <dbus/dbus.h>

#include <string>
#include <vector>

namespace HubPlatform {

class Proxy {
    friend bool operator==( const GDBusProxy* proxyPtr, const Proxy& proxyObj );
    friend bool operator==( const Proxy& proxyObj, const GDBusProxy* proxyPtr );

public:
    explicit Proxy( GDBusProxy* proxy );
    virtual ~Proxy();

    std::string getInterface() const;
    std::string getProxyPath() const;
    GDBusProxy* getProxyPtr() const;

protected:
    template < typename T >
    T getProperty( const std::string& propertyName ) const;

    template < typename T >
    std::vector< T > getArrayProperty( const std::string& propertyName ) const;

    void setBooleanProperty( const std::string& propertyName, bool value );

private:
    template < typename T >
    T getProperty( DBusMessageIter* iter ) const;

    template < typename T >
    static void validatePropertyType( DBusMessageIter* iter );

private:
    GDBusProxy* m_proxy;
};

template < typename T >
inline T Proxy::getProperty( const std::string& propertyName ) const {
    DBusMessageIter iter;
    if( g_dbus_proxy_get_property( getProxyPtr(), propertyName.c_str(), &iter ) == FALSE ) {
        throw BadProperty();
    }

    return getProperty< cpp_type_to_dbus_type_t< T > >( &iter );
}

template < typename T >
inline T Proxy::getProperty( DBusMessageIter* iter ) const {
    validatePropertyType< T >( iter );

    T propValue;
    dbus_message_iter_get_basic( iter, &propValue );

    return propValue;
}

inline void Proxy::setBooleanProperty( const std::string& propertyName, bool value ) {
    if( g_dbus_proxy_set_property_basic(
            getProxyPtr(), propertyName.c_str(), DBUS_TYPE_BOOLEAN, &value, nullptr, nullptr, nullptr ) == FALSE ) {
        throw BadProperty();
    }
}

template < typename T >
inline void Proxy::validatePropertyType( DBusMessageIter* iter ) {
    switch( dbus_message_iter_get_arg_type( iter ) ) {
    case DBUS_TYPE_STRING:
    case DBUS_TYPE_OBJECT_PATH:
        if( std::is_same< const char*, T >::value ) {
            return;
        }
        break;

    case DBUS_TYPE_BOOLEAN:
        if( std::is_same< dbus_bool_t, T >::value ) {
            return;
        }
        break;

    case DBUS_TYPE_UINT32:
        if( std::is_same< uint32_t, T >::value ) {
            return;
        }
        break;

    case DBUS_TYPE_UINT16:
        if( std::is_same< uint16_t, T >::value ) {
            return;
        }
        break;

    case DBUS_TYPE_INT16:
        if( std::is_same< int16_t, T >::value ) {
            return;
        }
        break;

    case DBUS_TYPE_BYTE:
        if( std::is_same< unsigned char, T >::value ) {
            return;
        }
        break;

    default:
        break;
    }

    throw BadPropertyType();
}

template < typename T >
inline std::vector< T > Proxy::getArrayProperty( const std::string& propertyName ) const {
    DBusMessageIter iter;
    if( g_dbus_proxy_get_property( getProxyPtr(), propertyName.c_str(), &iter ) == FALSE ) {
        throw BadProperty();
    }

    DBusMessageIter subIter;
    dbus_message_iter_recurse( &iter, &subIter );

    std::vector< T > result;

    if( dbus_type_is_fixed( dbus_message_iter_get_arg_type( &subIter ) ) ) {
        int len = -1;
        cpp_type_to_dbus_type_t< T >* data;

        dbus_message_iter_get_fixed_array( &subIter, &data, &len );

        if( len > 0 ) {
            result.assign( data, data + len );
        }
    }
    else {
        while( dbus_message_iter_get_arg_type( &subIter ) != DBUS_TYPE_INVALID ) {
            result.push_back( getProperty< cpp_type_to_dbus_type_t< T > >( &subIter ) );
            dbus_message_iter_next( &subIter );
        }
    }

    return result;
}

}  // namespace HubPlatform
