#ifndef PARSER_H
#define PARSER_H

#include <functional>
#include <map>

template < typename T >
class Parser {
public:
    typedef std::function< T > HandlerPtr;
    typedef std::map< std::string, HandlerPtr > HandlerMap;

private:
    HandlerMap handlerMap_;
    HandlerPtr unknownMethod_;

public:
    Parser( const HandlerMap& handlerMap, const HandlerPtr& unknownMethod )
    : handlerMap_( handlerMap )
    , unknownMethod_( unknownMethod ) {}
    virtual ~Parser() = default;
    HandlerPtr getHandler( const std::string& strMethodName, bool& bMethodWasFound ) {
        bMethodWasFound = false;
        typename HandlerMap::iterator itFind = handlerMap_.find( strMethodName ), itEnd = handlerMap_.end();
        if( itFind != itEnd ) {
            bMethodWasFound = true;
            return itFind->second;
        }
        return unknownMethod_;
    }
    HandlerPtr getHandler( const std::string& strMethodName ) {
        bool bMethodWasFound = false;
        return getHandler( strMethodName, bMethodWasFound );
    }
    bool has( const std::string& strMethodName ) const {
        typename HandlerMap::const_iterator itFind = handlerMap_.find( strMethodName ), itEnd = handlerMap_.cend();
        if( itFind != itEnd )
            return true;
        return false;
    }
};  /// template class Parser

#endif  // PARSER_H
