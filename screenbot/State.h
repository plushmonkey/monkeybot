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

class PathingState : public api::State {
private:
    DWORD m_StuckTimer;

protected:
    std::vector<Vec2> m_Plan;
    void UpdateStuckCheck(DWORD dt);
    void SmoothPath(const Level& level, const Pathing::Plan& plan, std::vector<Vec2>& result);
    void UpdatePath(Vec2 target, DWORD dt);
    double GetPlanDistance();

public:
    PathingState(api::Bot* bot);
    virtual ~PathingState() { }
};

class ChaseState : public PathingState {
private:
    DWORD m_LastEnemySeen;
    Vec2 m_LastRealEnemyCoord;

public:
    ChaseState(api::Bot* bot);
    ~ChaseState();

    void UpdateWeapons(DWORD dt);
    virtual void Update(DWORD dt);
    virtual api::StateType GetType() const { return api::StateType::ChaseState; }
};

class BaseduelState : public api::State {
private:
public:
    BaseduelState(api::Bot* bot);
    virtual void Update(DWORD dt);
    virtual api::StateType GetType() const { return api::StateType::BaseduelState; }
};

class PatrolState : public PathingState {
private:
    std::vector<Vec2> m_Waypoints;
    std::vector<Vec2> m_FullWaypoints;
    DWORD m_LastBullet;
    unsigned int m_CloseDistance;

    bool UpdateWaypoints();

public:
    PatrolState(api::Bot* bot, std::vector<Vec2> waypoints = std::vector<Vec2>(), unsigned int close_distance = 25);
    ~PatrolState();

    virtual void Update(DWORD dt);
    void ResetWaypoints(bool full = true);
    virtual api::StateType GetType() const { return api::StateType::PatrolState; }
};

class FollowState : public PathingState {
private:
    weak_ptr<Player> m_FollowPlayer;

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
    ~AggressiveState();
    virtual void Update(DWORD dt);

    virtual api::StateType GetType() const { return api::StateType::AggressiveState; }
};

#endif
