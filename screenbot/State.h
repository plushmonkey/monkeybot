#ifndef STATE_H_
#define STATE_H_

#include "Keyboard.h"
#include "Common.h"
#include "Pathing.h"

#include <vector>

class Bot;

enum class StateType {
    ChaseState,
    PatrolState,
    AggressiveState,
    AttachState,
    BaseduelState
};

std::ostream& operator<<(std::ostream& out, StateType type);

class State {
protected:
    Bot& m_Bot;

public:
    State(Bot& bot);
    virtual ~State() { }

    virtual void Update(DWORD dt) = 0;
    virtual StateType GetType() const = 0;
};
typedef std::shared_ptr<State> StatePtr;

class AttachState : public State {
private:
    Direction m_Direction;
    int m_Count;

public:
    AttachState(Bot& bot);
    virtual void Update(DWORD dt);
    virtual StateType GetType() const { return StateType::AttachState; }
};

class ChaseState : public State {
private:
    Pathing::Plan m_Plan;
    DWORD m_StuckTimer;
    Vec2 m_LastCoord;
    Vec2 m_LastRealEnemyCoord;

public:
    ChaseState(Bot& bot);
    virtual void Update(DWORD dt);
    virtual StateType GetType() const { return StateType::ChaseState; }
};

class BaseduelState : public State {
private:
public:
    BaseduelState(Bot& bot);
    virtual void Update(DWORD dt);
    virtual StateType GetType() const { return StateType::BaseduelState; }
};

class PatrolState : public State {
private:
    std::vector<Vec2> m_Waypoints;
    std::vector<Vec2> m_FullWaypoints;
    Pathing::Plan m_Plan;
    DWORD m_LastBullet;
    Vec2 m_LastCoord;
    DWORD m_StuckTimer;

public:
    PatrolState(Bot& bot, std::vector<Vec2> waypoints = std::vector<Vec2>());
    virtual void Update(DWORD dt);
    void ResetWaypoints(bool full = true);
    virtual StateType GetType() const { return StateType::PatrolState; }
};

class AggressiveState : public State {
private:
    Vec2 m_LastEnemyPos;
    DWORD m_NearWall;
    DWORD m_BurstTimer;
	DWORD m_DecoyTimer;

public:
    AggressiveState(Bot& bot);
    virtual void Update(DWORD dt);

    virtual StateType GetType() const { return StateType::AggressiveState; }
};

#endif
