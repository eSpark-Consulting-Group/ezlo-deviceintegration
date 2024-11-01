#pragma once

#include "cored/model/values/value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class LedSegmentValue : public Value {
public:
    static std::unique_ptr< LedSegmentValue > createValue( const Json& value );
    static std::unique_ptr< LedSegmentValue > createDefaultValue();

    ~LedSegmentValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

protected:
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    LedSegmentValue( std::string segmentName, uint32_t segmentStart, uint32_t segmentEnd );

private:
    std::string m_segmentName;
    uint32_t m_segmentStart;
    uint32_t m_segmentEnd;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
