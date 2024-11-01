#pragma once
#include "cored/model/values/value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class WeeklyIntervalValue : public Value {
public:
    static std::unique_ptr< WeeklyIntervalValue > createValue( const Json& value );
    static std::unique_ptr< WeeklyIntervalValue > createDefaultValue();

    ~WeeklyIntervalValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

protected:
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    using WeekDays = std::vector< std::string >;

    WeeklyIntervalValue( WeekDays&& weekDays, std::string&& startTime, std::string&& stopTime );

private:
    WeekDays m_weekDays;
    std::string m_startTime;
    std::string m_stopTime;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
