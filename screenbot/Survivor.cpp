#include "Survivor.h"

#include "Bot.h"
#include "Client.h"

#include <string>
#include <sstream>
#include <algorithm>

SurvivorGame::SurvivorGame(Bot* bot) 
    : m_Bot(bot),
      m_AliveTime(0),
      m_Kills(0),
      m_Announce(false)
{
    
}

void SurvivorGame::SetTarget(const std::string& target) {
    m_Target = target;

    std::transform(m_Target.begin(), m_Target.end(), m_Target.begin(), tolower);

    m_AliveTime = 0;
    m_Kills = 0;
    m_Announce = false;
}

void SurvivorGame::HandleMessage(KillMessage* mesg) {
    std::string killer = mesg->GetKiller();
    
    std::transform(killer.begin(), killer.end(), killer.begin(), tolower);

    if (killer.compare(m_Target) != 0) return;

    m_Kills++;
}

void SurvivorGame::AnnounceDeath() {
    if (m_Announce && m_AliveTime >= 10000) {
        /* Last target was priority target, announce death */
        int seconds = m_AliveTime / 1000;
        int mins = seconds / 60;
        seconds -= mins * 60;

        /* Find priority player's name to get formatted name */
        PlayerList players = m_Bot->GetMemorySensor().GetPlayers();
        PlayerList::const_iterator priority_player = std::find_if(players.begin(), players.end(), [&](PlayerPtr p) {
            std::string name = p->GetName();
            std::transform(name.begin(), name.end(), name.begin(), tolower);
            return name.compare(m_Target) == 0;
        });

        /* Read any kill messages in log so the kill count is correct */
        m_Bot->ForceLogRead();

        std::stringstream ss;

        /* Output formatted name if found, use lowercase otherwise */
        if (priority_player != players.end())
            ss << (*priority_player)->GetName();
        else
            ss << m_Target;

        ss << " survived for ";

        if (mins > 0)
            ss << mins << (mins == 1 ? " minute " : " minutes ");
        ss << seconds << (seconds == 1 ? " second " : " seconds ");
        ss << "and got " << m_Kills << (m_Kills == 1 ? " kill." : " kills.");

        m_Bot->GetClient()->SendString(ss.str());
    }

    m_AliveTime = 0;
    m_Kills = 0;
    m_Announce = false;
}

void SurvivorGame::Update(DWORD dt) {
    if (m_Target.length() == 0) return;

    PlayerPtr enemy = m_Bot->GetEnemyTarget();
    if (!enemy)
        AnnounceDeath();

    std::string enemy_name = enemy->GetName();

    std::transform(enemy_name.begin(), enemy_name.end(), enemy_name.begin(), tolower);

    if (enemy_name.compare(m_Target) != 0) {
        AnnounceDeath();
    } else {
        m_AliveTime += dt;
        m_Announce = true;
    }
}
