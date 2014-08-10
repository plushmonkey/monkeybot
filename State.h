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
private:
    Coord m_LastEnemyPos;
    DWORD m_LastEnemyTimer;
    Coord m_EnemyVelocity;
    DWORD m_LastBomb;
    DWORD m_LastNonSafeTime;

public:
    AggressiveState(Bot& bot);
    virtual void Update();
};

class RunState : public State {
public:
    RunState(Bot& bot);
    virtual void Update();
};


#endif
