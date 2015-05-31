#include "api/Api.h"
#include "plugin/Plugin.h"
#include "Vector.h"

#include <iostream>
#include <algorithm>
#include <sstream>

/** 
 * Not finished. It will probably crash.
 */

class SurvivorPlugin;

class SurvivorCommand : public api::Command {
private:
    SurvivorPlugin* m_Plugin;

public:
    SurvivorCommand(SurvivorPlugin* plugin);

    std::string GetHelp() const { return "Sets the initial survivor target. Use !survivor without a target to disable."; }
    std::string GetPermission() const { return "survivor.cmd"; }

    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class SurvivorPlugin : public Plugin {
private:
    api::Bot* m_Bot;
    DWORD m_AliveTime;
    int m_Kills;
    bool m_Announce;
    std::string m_Target;
    std::string m_Killer;

    unsigned int DetermineReward() {
        const float BaseReward = 3000.0f;
        const float RewardPerSec = 2000.0f / 60.0f;
        const float RewardPerKill = 2000.0f;
        const unsigned int MaxReward = 50000;
        int seconds = int(m_AliveTime / 1000.0f);
        
        unsigned int reward = static_cast<unsigned int>(BaseReward + (RewardPerSec * seconds) + (RewardPerKill * m_Kills));
        reward = std::min(reward, MaxReward);
        return reward;
    }

    std::string TargetRandom() {
        api::PlayerList players = m_Bot->GetMemorySensor().GetPlayers();
        unsigned int freq = m_Bot->GetFreq();

        players.erase(std::remove_if(players.begin(), players.end(), [&](api::PlayerPtr player) {
            bool in_safe = m_Bot->GetClient()->IsInSafe(player->GetPosition() / 16, m_Bot->GetLevel());

            Vec2 pos = player->GetPosition() / 16;

            bool in_center = pos.x > 320 && pos.x < 703 && pos.y > 320 && pos.y < 703;

            return player->GetFreq() == freq || player->GetShip() == api::Ship::Spectator || in_safe || player->GetName()[0] == '<' || !in_center;
        }), players.end());

        if (players.size() == 0) return "";

        std::size_t target_index = rand() % players.size();
        return players.at(target_index)->GetName();
    }

    api::PlayerPtr FindPlayer(std::string name) {
        api::PlayerList players = m_Bot->GetMemorySensor().GetPlayers();

        std::transform(name.begin(), name.end(), name.begin(), tolower);

        api::PlayerList::const_iterator player = std::find_if(players.begin(), players.end(), [&](api::PlayerPtr p) {
            std::string find_name = p->GetName();
            std::transform(find_name.begin(), find_name.end(), find_name.begin(), tolower);
            return find_name.compare(name) == 0;
        });

        if (player == players.end()) return nullptr;

        return *player;
    }

    void AnnounceDeath() {
        if (m_Announce && m_AliveTime >= 10000) {
            /* Last target was survivor target, announce death */
            int seconds = m_AliveTime / 1000;
            int mins = seconds / 60;
            seconds -= mins * 60;

            /* Read any kill messages in log so the kill count is correct */
            m_Bot->UpdateLog();

            int reward = DetermineReward();

            std::string formatted_name = m_Target;
            api::PlayerPtr survivor = FindPlayer(m_Target);
            
            if (survivor)
                formatted_name = survivor->GetName();

            std::stringstream ss;
            ss << formatted_name;

            ss << " survived for ";

            if (mins > 0)
                ss << mins << (mins == 1 ? " minute " : " minutes ");
            ss << seconds << (seconds == 1 ? " second " : " seconds ");
            ss << "and got " << m_Kills << (m_Kills == 1 ? " kill." : " kills.");
            ss << " Reward: $" << reward << ".";

            std::string target = m_Killer;

            /* Set new target to the killer or select random target if no killer */
            if (target.length() == 0 || target.compare(m_Bot->GetName()) == 0 || target.at(0) == '<')
                target = TargetRandom();

            api::PlayerPtr target_player = FindPlayer(target);
            if (!target_player || target_player->GetFreq() == m_Bot->GetFreq())
                target = TargetRandom();

            ss << " New target: " << target << "." << std::endl;

            m_Bot->GetClient()->SendString(ss.str());
            m_Bot->GetClient()->SendPM(m_Target, "?give " + std::to_string(reward));

            SetTarget(target);
        }

        m_AliveTime = 0;
        m_Kills = 0;
        m_Announce = false;
        m_Killer = "";
    }

public:
    SurvivorPlugin(api::Bot* bot)
        : m_Bot(bot),
          m_AliveTime(0),
          m_Kills(0),
          m_Announce(false)
    {

    }

    int OnCreate() {
        std::string version = m_Bot->GetVersion().GetString();

        std::cout << "Survivor loaded for monkeybot version " << version << std::endl;

        m_Bot->GetCommandHandler().RegisterCommand("survivor", api::CommandPtr(new SurvivorCommand(this)));

        return 0;
    }

    int OnUpdate(unsigned long dt) {
        if (m_Target.length() == 0) return 0;

        api::PlayerPtr enemy = m_Bot->GetEnemyTarget();
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

        return 0;
    }

    int OnDestroy() {
        std::shared_ptr<api::EnemySelectorFactory> factory = m_Bot->GetEnemySelectors();
        api::SelectorPtr selector = factory->CreateClosest();

        m_Bot->SetEnemySelector(selector);
        m_Bot->GetCommandHandler().UnregisterCommand("survivor");
        return 0;
    }

    void OnChatMessage(ChatMessage* mesg) {
        
    }

    void OnKill(KillMessage* mesg) {
        std::string killer = mesg->GetKiller();
        std::string killed = mesg->GetKilled();

        std::transform(killer.begin(), killer.end(), killer.begin(), tolower);
        std::transform(killed.begin(), killed.end(), killed.begin(), tolower);

        if (killed.compare(m_Target) == 0 && killer.compare(m_Target) != 0)
            m_Killer = mesg->GetKiller();

        if (killer.compare(m_Target) != 0) return;

        m_Kills++;
    }

    void SetTarget(const std::string& target) {
        m_Target = target;

        std::transform(m_Target.begin(), m_Target.end(), m_Target.begin(), tolower);

        m_AliveTime = 0;
        m_Kills = 0;
        m_Announce = false;
        m_Killer = "";

        std::shared_ptr<api::EnemySelectorFactory> factory = m_Bot->GetEnemySelectors();
        api::SelectorPtr selector;

        if (m_Target.length() > 0) {
            selector = factory->CreateTarget(m_Target);
            m_Bot->GetClient()->SendString(";!target " + m_Target);
        } else {
            selector = factory->CreateClosest();
        }

        m_Bot->SetEnemySelector(selector);
    }
};

SurvivorCommand::SurvivorCommand(SurvivorPlugin* plugin)
    : m_Plugin(plugin)
{
    
}

void SurvivorCommand::Invoke(api::Bot* bot, const std::string& sender, const std::string& args) {
    m_Plugin->SetTarget(args);
}

extern "C" {
    PLUGIN_FUNC Plugin* CreatePlugin(api::Bot* bot) {
        return new SurvivorPlugin(bot);
    }

    PLUGIN_FUNC void DestroyPlugin(Plugin* plugin) {
        delete plugin;
    }

    PLUGIN_FUNC const char* GetPluginName() {
        return "Survivor";
    }
}
