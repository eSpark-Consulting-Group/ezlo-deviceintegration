#pragma once

#define METHOD_MACRO( _name, _in_args, _out_args, _class, _function )                        \
    name:                                                                                    \
    _name,                                                                                   \
        function :                                                                           \
        []( DBusConnection* connection, DBusMessage* message, void* data ) -> DBusMessage* { \
            auto obj = static_cast< _class* >( data );                                       \
            return obj->_function( connection, message );                                    \
        },                                                                                   \
        flags : static_cast< GDBusMethodFlags >( 0 ),                                        \
                privilege : 0,                                                               \
                in_args : _in_args,                                                          \
                          out_args : _out_args

#define ASYNC_METHOD_MACRO( _name, _in_args, _out_args, _class, _function )                  \
    name:                                                                                    \
    _name,                                                                                   \
        function :                                                                           \
        []( DBusConnection* connection, DBusMessage* message, void* data ) -> DBusMessage* { \
            auto obj = static_cast< _class* >( data );                                       \
            return obj->_function( connection, message );                                    \
        },                                                                                   \
        flags : G_DBUS_METHOD_FLAG_ASYNC,                                                    \
                privilege : 0,                                                               \
                in_args : _in_args,                                                          \
                          out_args : _out_args

#define PROPERTY_MACRO( _name, _type, _class, _function )                                                         \
    name:                                                                                                         \
    _name, type : _type,                                                                                          \
                  get : []( const GDBusPropertyTable* property, DBusMessageIter* iter, void* data ) -> gboolean { \
                      auto obj = static_cast< _class* >( data );                                                  \
                      return obj->_function( property, iter );                                                    \
                  }
