#include "Taunter.h"

#include "Bot.h"
#include "Client.h"
#include "Random.h"
#include "Util.h"

#include <iostream>
#include <regex>
#include <Windows.h>
#include <algorithm>

void Taunter::HandleMessage(KillMessage* mesg) {
    if (!m_Enabled) return;

    DWORD current_time = timeGetTime();

    if (current_time - m_LastTaunt < m_Bot->GetConfig().TauntCooldown) return;

    std::string bot_name = m_Bot->GetName();
    std::string killer = mesg->GetKiller();
    std::string killed = mesg->GetKilled();
    std::string bounty = std::to_string(mesg->GetBounty());

    const auto& taunts = m_Bot->GetConfig().Taunts;
    const auto& whitelist = m_Bot->GetConfig().TauntWhitelist;

    if (taunts.size() == 0) return;

    unsigned int tid = Random::GetU32(0, taunts.size() - 1);
    std::string tosend = taunts.at(tid);

    if (bot_name.length() == 0) {
        std::cout << "Bot name not found. Set it in config." << std::endl;
        return;
    }

    std::string killer_lower(killer);
    std::string killed_lower(killed);
    std::string name_lower(bot_name);

    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), tolower);
    std::transform(killer_lower.begin(), killer_lower.end(), killer_lower.begin(), tolower);
    std::transform(killed_lower.begin(), killed_lower.end(), killed_lower.begin(), tolower);

    if (killer_lower.compare(name_lower) != 0) return;
    if (killed_lower.compare(name_lower) == 0) return;
    if (killed.length() > 0 && killed.at(0) == '<') return;

    bool skip = false;
    for (auto& name : whitelist) {
        if (killed_lower.compare(name) == 0) {
            skip = true;
            break;
        }
    }
    if (skip) return;

    Util::str_replace(tosend, "{name}", killed);
    Util::str_replace(tosend, "{bounty}", bounty);

    m_Bot->GetClient()->SendString(tosend);
    m_LastTaunt = current_time;
}

Taunter::Taunter(Bot* bot)
    : m_Bot(bot),
      m_LastTaunt(0),
      m_Enabled(true)
{
    
}
