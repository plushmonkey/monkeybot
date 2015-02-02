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
    int m_SpawnX;
    int m_SpawnY;
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
    int m_MinGunRange;

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
    bool m_Patrol;
    bool m_Attach;
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
    DWORD m_LastEnemyTimer;
    Vec2 m_EnemyVelocity;
    DWORD m_LastNonSafeTime;
    DWORD m_NearWall;
    DWORD m_BurstTimer;
	DWORD m_DecoyTimer;

    // Config
    int m_RunPercent;
    int m_XPercent;
    int m_SafeResetTime;
    int m_TargetDist;
    int m_RunDist;
    int m_StopBombing;
    int m_DistFactor;
    bool m_OnlyCenter;
    bool m_Patrol;
    int m_MinGunRange;
    bool m_Baseduel;
    int m_ProjectileSpeed;
    int m_IgnoreDelayDistance;
    bool m_UseBurst;
	unsigned int m_DecoyDelay;

public:
    AggressiveState(Bot& bot);
    virtual void Update(DWORD dt);

    virtual StateType GetType() const { return StateType::AggressiveState; }
};

#endif
