#pragma once

#include "scenes_manager/scenes_engine/events/item_event_base.h"
#include "scenes_manager/scenes_engine/events/item_value_provider.h"

namespace SceneEngine {

class InArrayEvent : public ItemEventBase {
public:
    using ValuesArray = std::vector< ItemValue >;

    InArrayEvent( ValuesArray array, const std::string& operation );
    ~InArrayEvent() override = default;

    std::string getMethodName() const override;

    bool compare( const ItemValue& itemValue ) const override;

    void checkValue( const ItemValue& leftValue ) const override;

    void setKeepState( bool value );

private:
    static bool checkOperation( const ItemValue& value, const ValuesArray& array, const std::string& operation );

    static void validateOperation( const std::string& operation );
    static void validateArray( const ValuesArray& array );

private:
    ValuesArray m_array;
    std::string m_operation;

    mutable bool m_isTriggered{ false };
    bool m_keepState{ true };
};

using InArrayEventPtr = std::shared_ptr< InArrayEvent >;

}  // namespace SceneEngine
