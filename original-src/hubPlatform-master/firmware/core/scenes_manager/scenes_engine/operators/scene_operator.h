#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include "scenes_manager/scenes_engine/operators/visitors/operator_visitor.h"

namespace SceneEngine {

class AbstractOperator : public std::enable_shared_from_this< AbstractOperator > {
public:
    enum OperatorState { Disabled, True, False };

    using SharedOperator = std::shared_ptr< AbstractOperator >;
    using OperatorStateEventHandler = std::function< void( OperatorState newState ) >;
    using SceneStateEventHandler = std::function< void( OperatorState newState ) >;

    virtual ~AbstractOperator() = default;
    virtual OperatorState checkParent( SharedOperator child, OperatorState state );
    virtual OperatorState checkChildren() = 0;
    virtual void registerChild( SharedOperator child ) = 0;
    virtual void accept( OperatorVisitor& visitor ) = 0;

    /// @brief If operator is disabled then it is TRUE all the time.
    /// State setting does not change anything.
    void setEnabled( bool enabled );
    bool isEnabled() const;

    void setParent( SharedOperator parent );
    SharedOperator getParent() const;

    void setBlockID( std::string blockId );
    const std::string& getBlockID() const;

    void setBlockName( std::string name );
    const std::string& getBlockName() const;

    void setOperatorStateEventHandler( OperatorStateEventHandler );
    void setSceneStateEventHandler( SceneStateEventHandler );
    void setSceneStateEventHandlerDisabled( bool value );
    bool isSceneStateEventHandlerDisabled() const;

    static bool toBool( OperatorState );
    static OperatorState toOperatorState( bool );

protected:
    void notifyOperatorStateEvent( OperatorState state );
    void notifySceneStateEvent( OperatorState state );

    SharedOperator m_parent;
    bool m_isEnabled = true;
    std::string m_blockID;
    std::string m_blockName;

    OperatorStateEventHandler m_operatorStateEventHandler;
    SceneStateEventHandler m_sceneStateEventHandler;
    bool m_sceneStateEventHandlerDisabled{ false };
};

std::string operatorStateToString( AbstractOperator::OperatorState state );
bool operatorStateFromString( const std::string& str, AbstractOperator::OperatorState& state );

}  // namespace SceneEngine
