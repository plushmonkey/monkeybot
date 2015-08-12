#ifndef STATE_H_
#define STATE_H_

#include "Keyboard.h"
#include "Common.h"
#include "Pathing.h"

#include <vector>

class Player;

std::ostream& operator<<(std::ostream& out, api::StateType type);

class AttachState : public api::State {
private:
    Direction m_Direction;
    int m_Count;

public:
    AttachState(api::Bot* bot);
    void Update(DWORD dt);
    api::StateType GetType() const { return api::StateType::AttachState; }
};

class ChaseState : public api::State {
private:
    Pathing::Plan m_Plan;
    DWORD m_StuckTimer;
    DWORD m_LastEnemySeen;
    Vec2 m_LastCoord;
    Vec2 m_LastRealEnemyCoord;

public:
    ChaseState(api::Bot* bot);
    virtual void Update(DWORD dt);
    virtual api::StateType GetType() const { return api::StateType::ChaseState; }

    const Pathing::Plan& GetPlan() const { return m_Plan; }
};

class BaseduelState : public api::State {
private:
public:
    BaseduelState(api::Bot* bot);
    virtual void Update(DWORD dt);
    virtual api::StateType GetType() const { return api::StateType::BaseduelState; }
};

class PatrolState : public api::State {
private:
    std::vector<Vec2> m_Waypoints;
    std::vector<Vec2> m_FullWaypoints;
    Pathing::Plan m_Plan;
    DWORD m_LastBullet;
    Vec2 m_LastCoord;
    DWORD m_StuckTimer;
    unsigned int m_CloseDistance;

public:
    PatrolState(api::Bot* bot, std::vector<Vec2> waypoints = std::vector<Vec2>(), unsigned int close_distance = 25);
    virtual void Update(DWORD dt);
    void ResetWaypoints(bool full = true);
    virtual api::StateType GetType() const { return api::StateType::PatrolState; }
};

class FollowState : public api::State {
private:
    weak_ptr<Player> m_FollowPlayer;
    Pathing::Plan m_Plan;
    DWORD m_StuckTimer;

public:
    FollowState(api::Bot* bot, std::string follow);
    virtual void Update(DWORD dt);

    virtual api::StateType GetType() const { return api::StateType::FollowState; }
};

class AggressiveState : public api::State {
private:
    Vec2 m_LastEnemyPos;
    DWORD m_NearWall;
    DWORD m_BurstTimer;
	DWORD m_DecoyTimer;
    DWORD m_RocketTimer;
    DWORD m_LastSafeTime;

public:
    AggressiveState(api::Bot* bot);
    virtual void Update(DWORD dt);

    virtual api::StateType GetType() const { return api::StateType::AggressiveState; }
};

#endif
