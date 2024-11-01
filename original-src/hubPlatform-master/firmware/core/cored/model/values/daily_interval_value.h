#pragma once
#include "cored/model/values/value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class DailyIntervalValue : public Value {
public:
    static std::unique_ptr< DailyIntervalValue > createValue( const Json& value );
    static std::unique_ptr< DailyIntervalValue > createDefaultValue();

    ~DailyIntervalValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

protected:
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    DailyIntervalValue( std::string&& startDateTime, std::string&& stopDateTime );

private:
    std::string m_startDateTime;
    std::string m_stopDateTime;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
