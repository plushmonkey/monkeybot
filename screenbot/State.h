#ifndef STATE_H_
#define STATE_H_

#include "Keyboard.h"
#include "Common.h"
#include <vector>

class Bot;

class State {
protected:
    Bot& m_Bot;

public:
    State(Bot& bot);
    virtual ~State() { }

    virtual void Update(DWORD dt) = 0;
};
typedef std::shared_ptr<State> StatePtr;

class FollowState : public State {
public:
    FollowState(Bot& bot);
    virtual void Update(DWORD dt);
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
    
    std::vector<unsigned> m_PossibleAddr;

public:
    AggressiveState(Bot& bot);
    virtual void Update(DWORD dt);

    void SetPossibleAddr(std::vector<unsigned> addr) { m_PossibleAddr = addr; }
};

class RunState : public State {
public:
    RunState(Bot& bot);
    virtual void Update(DWORD dt);
};

class MemoryState : public State {
private:
    // Address -> Value
    std::map<unsigned int, unsigned int> m_FindSpace;

    bool m_Up;

public:
    MemoryState(Bot& bot);
    virtual void Update(DWORD dt);
};

#endif
