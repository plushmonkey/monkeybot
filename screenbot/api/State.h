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

} // ns

#endif
