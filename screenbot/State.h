#ifndef STATE_H_
#define STATE_H_

#include "Keyboard.h"
#include "Common.h"
#include <vector>
#include "Pathing.h"

class Bot;

enum class StateType {
    MemoryState,
    FollowState,
    PatrolState,
    AggressiveState
};

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

class FollowState : public State {
private:
    Pathing::Plan m_Plan;

public:
    FollowState(Bot& bot);
    virtual void Update(DWORD dt);
    virtual StateType GetType() const { return StateType::FollowState; }
};

class PatrolState : public State {
private:
    std::vector<Coord> m_Waypoints;
    Pathing::Plan m_Plan;
    DWORD m_LastBullet;
    bool m_Patrol;

    Coord m_LastCoord;
    DWORD m_StuckTimer;

public:
    PatrolState(Bot& bot, std::vector<Coord> waypoints = std::vector<Coord>());
    virtual void Update(DWORD dt);
    void ResetWaypoints(bool full = true);
    virtual StateType GetType() const { return StateType::PatrolState; }
};

class AggressiveState : public State {
private:
    Coord m_LastEnemyPos;
    DWORD m_LastEnemyTimer;
    Coord m_EnemyVelocity;
    DWORD m_LastNonSafeTime;
    DWORD m_NearWall;

    // Config
    int m_RunPercent;
    int m_XPercent;
    int m_SafeResetTime;
    int m_TargetDist;
    int m_RunDist;
    int m_StopBombing;
    int m_DistFactor;
    bool m_MemoryScanning;
    bool m_OnlyCenter;
    bool m_Patrol;

public:
    AggressiveState(Bot& bot);
    virtual void Update(DWORD dt);

    virtual StateType GetType() const { return StateType::AggressiveState; }
};

class MemoryState : public State {
private:
    // Address -> Value
    std::map<unsigned int, unsigned int> m_FindSpace;

    bool m_Up;

public:
    MemoryState(Bot& bot);
    virtual void Update(DWORD dt);
    virtual StateType GetType() const { return StateType::MemoryState; }
};

#endif
