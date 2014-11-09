#ifndef STATE_H_
#define STATE_H_

#include "Keyboard.h"
#include "Common.h"
#include <vector>
#include "Pathing.h"

class Bot;

enum class StateType {
    MemoryState,
    ChaseState,
    PatrolState,
    AggressiveState
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

class ChaseState : public State {
private:
    Pathing::Plan m_Plan;
    DWORD m_StuckTimer;
    Coord m_LastCoord;
    Coord m_LastRealEnemyCoord;

public:
    ChaseState(Bot& bot);
    virtual void Update(DWORD dt);
    virtual StateType GetType() const { return StateType::ChaseState; }
};

class PatrolState : public State {
private:
    std::vector<Coord> m_Waypoints;
    std::vector<Coord> m_FullWaypoints;
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
    Coord m_LastRealEnemyCoord;

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
    int m_MinGunRange;

public:
    AggressiveState(Bot& bot);
    virtual void Update(DWORD dt);

    virtual StateType GetType() const { return StateType::AggressiveState; }
};

class MemoryState : public State {
private:
    // Address -> Value
    std::map<unsigned int, unsigned int> m_FindSpace;
    int m_SpawnX;
    int m_SpawnY;
    bool m_Up;

public:
    MemoryState(Bot& bot);
    virtual void Update(DWORD dt);
    virtual StateType GetType() const { return StateType::MemoryState; }
};

#endif
