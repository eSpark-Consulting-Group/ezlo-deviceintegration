#pragma once

#include "scenes_manager/scenes_engine/events/event_base.h"
#include <set>

namespace SceneEngine {

class DictionaryValueEvent : public EventBase {
public:
    using Operation = std::string;
    using ElementValue = JsonObject;
    using Operations = std::set< Operation >;

    DictionaryValueEvent( const ElementValue& elementValue );
    DictionaryValueEvent( ElementValue&& elementValue );

    std::string getMethodName() const override;

    virtual bool compare( const Operation& operation, const ElementValue& element ) const;

    const ElementValue& getElementValue() const;

private:
    ElementValue m_elementValue;
    const Operations m_operations;
};

using DictionaryValueEventPtr = std::shared_ptr< DictionaryValueEvent >;

}  // namespace SceneEngine
