#ifndef SURVIVOR_H_
#define SURVIVOR_H_

#include "MessageQueue.h"
#include "Messages/KillMessage.h"

#include <Windows.h>

class Bot;

class SurvivorGame : MessageHandler<KillMessage> {
private:
    DWORD m_AliveTime;
    Bot* m_Bot;
    int m_Kills;
    bool m_Announce;
    std::string m_Target;

    void HandleMessage(KillMessage* mesg);
    void AnnounceDeath();

public:
    SurvivorGame(Bot* bot);
    void Update(DWORD dt);

    void SetTarget(const std::string& target);
};

#endif
