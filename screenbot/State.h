#ifndef STATE_H_
#define STATE_H_

#include "Keyboard.h"
#include "Common.h"
#include <vector>
#include <queue>
#include "Pathing.h"

class Bot;

enum class StateType {
    MemoryState,
    FollowState,
    PatrolState,
    AggressiveState,
    RunState
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
public:
    FollowState(Bot& bot);
    virtual void Update(DWORD dt);
    virtual StateType GetType() const { return StateType::FollowState; }
};

class PatrolState : public State {
private:
    Path::Graph m_Graph;
    std::queue<Coord> m_Waypoints;
    DWORD m_LastBullet;
    bool m_Patrol;

    Coord m_LastCoord;
    DWORD m_StuckTimer;

public:
    PatrolState(Bot& bot, std::queue<Coord> waypoints = std::queue<Coord>());
    virtual void Update(DWORD dt);
    void ResetWaypoints();
    virtual StateType GetType() const { return StateType::PatrolState; }
};

class AggressiveState : public State {
private:
    Coord m_LastEnemyPos;
    DWORD m_LastEnemyTimer;
    Coord m_EnemyVelocity;
    DWORD m_LastBomb;
    DWORD m_LastNonSafeTime;
    DWORD m_LastBullet;
    DWORD m_NearWall;
    bool m_FiringGun;
    int m_CurrentBulletDelay;
    DWORD m_TotalTimer;

    // Config
    int m_RunPercent;
    int m_XPercent;
    int m_SafeResetTime;
    int m_TargetDist;
    int m_RunDist;
    int m_StopBombing;
    int m_BombTime;
    bool m_FireBombs;
    bool m_FireGuns;
    int m_DistFactor;
    int m_BulletDelay;
    bool m_ScaleDelay;
    bool m_MemoryScanning;
    bool m_OnlyCenter;
    bool m_Patrol;
    
    std::vector<unsigned> m_PossibleAddr;

public:
    AggressiveState(Bot& bot);
    virtual void Update(DWORD dt);

    void SetPossibleAddr(std::vector<unsigned> addr) { m_PossibleAddr = addr; }
    virtual StateType GetType() const { return StateType::AggressiveState; }
};

class RunState : public State {
public:
    RunState(Bot& bot);
    virtual void Update(DWORD dt);
    virtual StateType GetType() const { return StateType::RunState; }
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
