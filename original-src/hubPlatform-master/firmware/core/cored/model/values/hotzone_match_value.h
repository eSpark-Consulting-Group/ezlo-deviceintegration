#pragma once
#include "cored/model/values/value.h"

#include <set>

namespace HubPlatform {
namespace CoreD {
namespace Model {

class HotzoneMatchValue : public Value {
public:
    static std::unique_ptr< HotzoneMatchValue > createValue( const Json& value );
    static std::unique_ptr< HotzoneMatchValue > createDefaultValue();

    ~HotzoneMatchValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

public:
    enum class MatchPolicy { Any, AnyOf, AllOf, OneOf, NoneOf, None };
    using MatchList = std::set< std::string >;

    MatchPolicy getMatchPolicy() const;
    const MatchList& getMatchList() const;

    static std::string matchPolicyToStr( MatchPolicy policy );

protected:
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    HotzoneMatchValue( MatchPolicy matchPolicy, MatchList matchList );

private:
    MatchPolicy m_matchPolicy = MatchPolicy::Any;
    MatchList m_matchList;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
