#pragma once

#include <functional>

template < typename T, T MemberCallback >
class CCallbackWrapper;

template < typename Class, typename Ret, typename... Params, Ret ( Class::*MemberCallback )( Params... ) >
class CCallbackWrapper< Ret ( Class::* )( Params... ), MemberCallback > {
public:
    using CCallbackProto = Ret ( * )( Params... );
    using MemberCallbackProto = Ret ( Class::* )( Params... );

    explicit CCallbackWrapper( Class* self ) {
        _self = self;
    }

    CCallbackWrapper( const CCallbackWrapper< MemberCallbackProto, MemberCallback >& wrapper ) = delete;

    CCallbackWrapper( CCallbackWrapper< MemberCallbackProto, MemberCallback >&& wrapper ) = delete;

    CCallbackWrapper< MemberCallbackProto, MemberCallback >& operator=(
        const CCallbackWrapper< MemberCallbackProto, MemberCallback >& wrapper ) = delete;

    CCallbackWrapper< MemberCallbackProto, MemberCallback >& operator=(
        CCallbackWrapper< MemberCallbackProto, MemberCallback >&& wrapper ) = delete;

    ~CCallbackWrapper() = default;

    operator CCallbackProto() {
        return CCallbackWrapper< MemberCallbackProto, MemberCallback >::callbackWrapper;
    }

private:
    static Ret callbackWrapper( Params... params ) {
        return ( _self->*MemberCallback )( params... );
    }

private:
    static Class* _self;
};

template < typename Class, typename Ret, typename... Params, Ret ( Class::*MemberCallback )( Params... ) >
Class* CCallbackWrapper< Ret ( Class::* )( Params... ), MemberCallback >::_self;
