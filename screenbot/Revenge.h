#ifndef REVENGE_H_
#define REVENGE_H_

#include "Common.h"
#include "Messages/KillMessage.h"

class Bot;

/*
    Switches to a different ship after so many deaths, 
    then switches back after getting a few kills.
*/
class Revenge : public MessageHandler<KillMessage> {
private:
    
    const int m_MaxDeaths;
    const api::Ship m_StrongShip;
    Bot& m_Bot;
    unsigned int m_UpdateID;

    std::string m_PrevTarget;
    api::Ship m_NormalShip;
    int m_DeathCounter;
    bool m_Enabled;
    DWORD m_LastDeath;

    bool IsWhitelisted(const std::string& name);

    bool OnUpdate(api::Bot* bot, unsigned long dt);
    void OnDeath(KillMessage *mesg);
    void OnKill(KillMessage *mesg);
    void HandleMessage(KillMessage* mesg);

    void Reset();
    
public:
    void SetEnabled(bool enabled);
    bool IsEnabled() const { return m_Enabled; }

    Revenge(Bot& bot, api::Ship current_ship, api::Ship strong_ship, int max_deaths);
    ~Revenge();

};

#endif
