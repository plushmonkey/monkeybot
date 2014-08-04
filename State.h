#ifndef STATE_H_
#define STATE_H_

#include "Keyboard.h"
#include "Common.h"

class Bot;

class State {
protected:
    Bot& m_Bot;

public:
    State(Bot& bot);
    virtual ~State() { }

    virtual void Update() = 0;
};
typedef std::shared_ptr<State> StatePtr;

class AggressiveState : public State {
public:
    AggressiveState(Bot& bot);
    virtual void Update();
};


#endif
