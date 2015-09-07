#ifndef API_STATE_H_
#define API_STATE_H_

#include <memory>

namespace api {

class Bot;
enum class StateType {
    ChaseState,
    PatrolState,
    AggressiveState,
    AttachState,
    BaseduelState,
    FollowState,
    PluginState
};

class State {
protected:
    Bot* m_Bot;

public:
    State(Bot* bot) : m_Bot(bot) { }
    virtual ~State() { }

    virtual void Update(DWORD dt) = 0;
    virtual StateType GetType() const { return StateType::PluginState; }
};

typedef std::shared_ptr<State> StatePtr;

class StateMachine {
public:
    virtual void Update(DWORD dt) = 0;
    virtual void Push(StatePtr state) = 0;
    virtual StatePtr Pop() = 0;
    virtual StatePtr GetState() const = 0;
    virtual bool IsEmpty() const = 0;
    virtual void Clear() = 0;
};

typedef std::shared_ptr<StateMachine> StateMachinePtr;

} // ns

#endif
