#ifndef KILL_MESSAGE_H_
#define KILL_MESSAGE_H_

#include <string>
#include "../MessageQueue.h"

class KillMessage : public Message {
private:
    std::string m_Killer;
    std::string m_Killed;
    int m_Bounty;

public:
    KillMessage(const std::string& killer, const std::string& killed, int bounty) 
        : m_Killer(killer), m_Killed(killed), m_Bounty(bounty)
    {

    }

    const std::string& GetKiller() const { return m_Killer; }
    const std::string& GetKilled() const { return m_Killed; }
    int GetBounty() const { return m_Bounty; }
};

#endif
