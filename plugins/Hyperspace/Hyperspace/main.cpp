#include "api/Api.h"
#include "plugin/Plugin.h"
#include "Vector.h"
#include "Tokenizer.h"

/**
 * This has barely been tested. It might cause crashes or do random bad things.
 *
 * Flagging probably requires CenterOnly in config to be false.
 * This plugin doesn't alter the config yet, so it needs to be done before turning on flagging.
 */

#undef max

#include <iostream>
#include <memory>
#include <thread>
#include <regex>

namespace {

// List of friends to join if they are on a private frequency
const std::vector<std::string> Friends = { "taz", "kirino", "obama" };
// Password for the private frequency
const std::string Password = "mmmm";

}

bool InRect(Vec2 pos, Vec2 min_rect, Vec2 max_rect) {
    return ((pos.x >= min_rect.x && pos.y >= min_rect.y) &&
            (pos.x <= max_rect.x && pos.y <= max_rect.y));
}

class HyperspaceSelector : public api::Selector {
public:
    std::shared_ptr<api::Player> Select(api::Bot* bot) {
        ClientPtr client = bot->GetClient();

        std::vector<api::PlayerPtr> enemies = client->GetEnemies();

        if (enemies.size() == 0) return api::PlayerPtr(nullptr);

        // Distance of closest enemy with multiplier applied
        double closest_calc_dist = std::numeric_limits<double>::max();
        api::PlayerPtr closest = api::PlayerPtr(nullptr);
        // Determines how much the rotation difference will increase distance by
        const double RotationMultiplier = 2.25;

        Vec2 bot_pos = bot->GetPos();
        bool has_x = (bot->GetMemorySensor().GetBotPlayer()->GetStatus() & 4) != 0;
        Vec2 half_resolution = Vec2(1920, 1080) / 2;
        Vec2 view_min = bot_pos - half_resolution / 16;
        Vec2 view_max = bot_pos + half_resolution / 16;

        Vec2 spawn(512, 512);
        const int center_radius = 512 - 320;

        bool center_only = InRect(bot->GetPos(), spawn - center_radius, spawn + center_radius);

        for (unsigned int i = 0; i < enemies.size(); ++i) {
            api::PlayerPtr& enemy = enemies.at(i);
            Vec2 enemy_pos = enemy->GetPosition() / 16;
            
            if (enemy_pos.x <= 0 && enemy_pos.y <= 0) continue;
            if (client->IsInSafe(enemy_pos, bot->GetLevel())) continue;

            double cdist = bot_pos.Distance(enemy_pos);

            if (cdist < 15 && bot->IsInSafe()) continue;

            bool in_center = InRect(enemy_pos, spawn - center_radius, spawn + center_radius);
            bool in_mini = InRect(enemy_pos, Vec2(460, 613), Vec2(534, 670));
            if ((center_only && !in_center) || in_mini) continue;

            unsigned char status = enemy->GetStatus();
            bool stealth = (status & 1) != 0;
            bool cloak = (status & 2) != 0;

            if (!has_x) {
                if (stealth && cloak) continue;
                bool visible = InRect(enemy_pos, view_min, view_max);

                if (stealth && !visible) continue;
            }

            // Unit vector pointing towards this enemy
            Vec2 to_target = Vec2Normalize(enemy_pos - bot_pos);
            double dot = bot->GetHeading().Dot(to_target);
            double multiplier = 1.0 + ((1.0 - dot) / RotationMultiplier);
            double calc_dist = cdist * multiplier;

            if (calc_dist < closest_calc_dist) {
                closest_calc_dist = calc_dist;
                closest = enemy;
            }
        }

        return closest;
    }
};

class PrivateFrequencyEnforcer {
private:
    api::Bot* m_Bot;
    bool m_JoinFriends;
    bool m_Override;

    api::PlayerPtr FindFriend() {
        api::PlayerList players = m_Bot->GetMemorySensor().GetPlayers();
        api::PlayerList::iterator iter;
        
        for (iter = players.begin(); iter != players.end(); ++iter) {
            api::PlayerPtr player = *iter;
            std::string name = player->GetName();

            if (player->GetShip() == api::Ship::Spectator || player->GetFreq() < 100) continue;

            for (std::string f : Friends) {
                if (f.compare(name) == 0)
                    return player;
            }
        }

        return nullptr;
    }

    bool GetFullEnergy() {
        using namespace std;
        using namespace api;

        if (!m_Bot->IsInSafe()) return false;

        shared_ptr<Client> client = m_Bot->GetClient();

        client->SetXRadar(false);
        client->Gun(GunState::Tap);

        while (!m_Bot->FullEnergy()) {
            client->Update(100);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return true;
    }

    bool JoinPriv() {
        using namespace std;
        using namespace api;

        shared_ptr<Client> client = m_Bot->GetClient();

        if (!GetFullEnergy()) return false;

        client->SendString("?createpriv " + Password);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        return true;
    }

    bool JoinFriend(api::PlayerPtr target) {
        using namespace std;
        using namespace api;

        shared_ptr<Client> client = m_Bot->GetClient();

        if (!GetFullEnergy()) return false;

        if (target) {
            client->SendPM(target->GetName(), "?join " + Password);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        return target != nullptr;
    }

    std::size_t GetFreqSize() {
        using namespace std;
        using namespace api;

        PlayerList freq_players = m_Bot->GetClient()->GetFreqPlayers(m_Bot->GetFreq());

        freq_players.erase(remove_if(freq_players.begin(), freq_players.end(), [](PlayerPtr player) {
            return player->GetName().at(0) == '<';
        }), freq_players.end());

        return freq_players.size();
    }

    // Gets the enemy freqs
    std::map<int, api::PlayerList> GetEnemyFreqs() {
        using namespace std;
        using namespace api;

        PlayerList players = m_Bot->GetMemorySensor().GetPlayers();
        unsigned int freq = m_Bot->GetFreq();

        players.erase(remove_if(players.begin(), players.end(), [&](PlayerPtr player) {
            bool own_team = player->GetFreq() == freq;
            bool fake = player->GetName().at(0) == '<';
            bool in_spec = player->GetShip() == Ship::Spectator;
            bool flagging = player->GetFreq() == 90 || player->GetFreq() == 91;

            return own_team || fake || in_spec || flagging;
        }), players.end());
        
        std::map<int, PlayerList> freqs;
        for (PlayerPtr player : players)
            freqs[player->GetFreq()].push_back(player);

        return freqs;
    }

    void UpdateJoinFriends() {
        if (m_Override) return;

        std::map<int, api::PlayerList> freqs = GetEnemyFreqs();
        std::size_t freq_count = freqs.size();

        for (auto& kv : freqs) {
            if (kv.second.size() >= 3) {
                m_JoinFriends = true;
                return;
            }
        }
        m_JoinFriends = false;
    }

public:
    PrivateFrequencyEnforcer(api::Bot* bot)
        : m_Bot(bot), m_JoinFriends(false), m_Override(true)
    {

    }

    void SetJoinFriends(bool v) { m_JoinFriends = v; }
    bool GetJoinFriends() const { return m_JoinFriends; }

    void SetOverride(bool v) { m_Override = v; }
    bool GetOverride() const { return m_Override; }

    
    void OnUpdate(unsigned long dt) {
        using namespace std;
        using namespace api;

        shared_ptr<Client> client = m_Bot->GetClient();
        unsigned int freq = m_Bot->GetFreq();
        Ship ship = m_Bot->GetMemorySensor().GetBotPlayer()->GetShip();

        if (ship == Ship::Spectator || !m_Bot->IsInSafe()) return;

        if (!GetOverride())
            UpdateJoinFriends();

        if (m_JoinFriends) {
            size_t freq_size = GetFreqSize();
            
            if (freq_size == 1 || freq < 100) {
                api::PlayerPtr target = FindFriend();
                if (target)
                    JoinFriend(target);
                else if (!target && freq < 100)
                    JoinPriv();
            }
        } else {
            size_t freq_size = GetFreqSize();
            
            if (freq_size > 1 || freq < 100)
                JoinPriv();
        }
    }
};

class JoinFriendsCommand : public api::Command {
private:
    std::shared_ptr<PrivateFrequencyEnforcer> m_PrivEnforcer;

public:
    JoinFriendsCommand(std::shared_ptr<PrivateFrequencyEnforcer> enforcer)
        : m_PrivEnforcer(enforcer)
    {

    }

    std::string GetHelp() const { return "Toggles whether or not this bot joins private frequencies of other bots."; }
    std::string GetPermission() const { return "hyperspace.joinfriends"; }

    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args) {
        bool join = !m_PrivEnforcer->GetJoinFriends();

        if (!args.empty()) {
            if (args[0] == 't' || args[0] == 'T')
                join = true;
            else if (args[0] == 'f' || args[0] == 'F')
                join = false;

            m_PrivEnforcer->SetOverride(true);
        } else {
            m_PrivEnforcer->SetOverride(false);
        }

        m_PrivEnforcer->SetJoinFriends(join);

        std::cout << "Join friends: " << std::boolalpha << join << std::endl;
    }
};

class AnchorSelector {
private:
    const unsigned long SendDelay = 1000 * 30;
    const unsigned long LancsTimeout = 7500;
    const unsigned long AnchorSyncTime = 1000;

    api::Bot* m_Bot;
    unsigned long m_SendTimer;
    unsigned long m_AttachCheckTimer;
    std::vector<std::string> m_Lancs;
    std::vector<std::string> m_Evokers;
    std::weak_ptr<api::Player> m_PrevAnchor;

    void UpdateAnchors(const std::string& line) {
        std::string regex_str = R"::(^\s*\(S|E\) (.+)$)::";

        int ship_num = (int)m_Bot->GetShip() + 1;
        if (ship_num > 6 || ship_num == 3 || ship_num == 4)
            regex_str = R"::(^\s*\(S\) (.+)$)::";

        std::regex summoner_regex(regex_str);
        Util::Tokenizer tokenizer(line);

        tokenizer(',');

        for (const std::string& anchor : tokenizer) {
            std::sregex_iterator begin(anchor.begin(), anchor.end(), summoner_regex);
            std::sregex_iterator end;

            if (begin != end) {
                size_t pos = anchor.find("(S)");
                if (pos != std::string::npos) {
                    m_Lancs.emplace_back(anchor.substr(anchor.find("(S) ") + 4));
                    continue;
                }

                pos = anchor.find("(E)");
                if (pos == std::string::npos) continue;
                m_Evokers.emplace_back(anchor.substr(anchor.find("(E) ") + 4));
            }
        }
    }

    api::PlayerPtr GetPlayerByName(const std::string& name) const {
        api::PlayerList players = m_Bot->GetMemorySensor().GetPlayers();

        auto iter = std::find_if(players.begin(), players.end(), [&](api::PlayerPtr player) -> bool {
            return player->GetName().compare(name) == 0;
        });

        if (iter == players.end())
            return nullptr;
        return *iter;
    }

    bool IsLanc(api::PlayerPtr player) const {
        return player->GetShip() == api::Ship::Lancaster;
    }

    bool IsEvoker(api::PlayerPtr player) const {
        return (player->GetShip() == api::Ship::Spider || player->GetShip() == api::Ship::Leviathan);
    }

public:
    AnchorSelector(api::Bot* bot) : m_Bot(bot), m_SendTimer(0), m_AttachCheckTimer(0), m_PrevAnchor() { }

    api::PlayerPtr GetAnchor() {
        api::PlayerPtr anchor;

        // Do evokers before lancs so lancs have priority
        for (const std::string& evoker : m_Evokers) {
            api::PlayerPtr player = GetPlayerByName(evoker);
            if (player && IsEvoker(player)) {
                anchor = player;
                break;
            }
        }

        for (const std::string& lanc : m_Lancs) {
            api::PlayerPtr player = GetPlayerByName(lanc);
            if (player && IsLanc(player)) {
                anchor = player;
                break;
            }
        }

        if (!anchor) {
            api::PlayerPtr prev = m_PrevAnchor.lock();
            // Only attach to previous anchor if they are still viable anchor
            if (prev && prev->GetFreq() == m_Bot->GetFreq() && (IsLanc(prev) || IsEvoker(prev)))
                anchor = prev;
        } else {
            m_PrevAnchor = anchor;
        }

        return anchor;
    }

    void ForceSend() {
        m_SendTimer = SendDelay;
    }

    void OnUpdate(unsigned long dt) {
        m_SendTimer += dt;

        if (m_SendTimer >= SendDelay) {
            std::cout << "Sending ?lancs" << std::endl;
            m_Bot->GetClient()->SendString("?lancs");
            m_Lancs.clear();
            m_Evokers.clear();
            m_SendTimer = 0;
        }

        api::PlayerPtr anchor = m_PrevAnchor.lock();

        if (anchor && m_Bot->GetEnergyPercent() > 0) {
            Vec2 botPos = m_Bot->GetMemorySensor().GetBotPlayer()->GetPosition();
            Vec2 anchorPos = anchor->GetPosition();

            // Detach bot from anchor if position has been in sync for some time
            if (botPos == anchorPos && botPos != Vec2(0, 0)) {
                m_AttachCheckTimer += dt;

                if (m_AttachCheckTimer >= AnchorSyncTime) {
                    m_Bot->GetClient()->Attach();
                    m_AttachCheckTimer = 0;
                }
            }
        }
    }

    void OnChatMessage(ChatMessage* mesg) {
        if (mesg->GetType() == ChatMessage::Type::Arena && (m_Lancs.size() == 0 && m_Evokers.size() == 0) && m_SendTimer < LancsTimeout)
            UpdateAnchors(mesg->GetMessage());
    }
};

class HyperspacePlugin;

class FlagCommand : public api::Command {
private:
    HyperspacePlugin* m_Plugin;

public:
    FlagCommand(HyperspacePlugin* plugin) : m_Plugin(plugin) {

    }

    std::string GetHelp() const { return "Makes the bot type ?flag and sets certain config options to make it work for flagging in hyperspace."; }
    std::string GetPermission() const { return "default.flag"; }
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class HyperspacePlugin : public Plugin {
private:
    api::Bot* m_Bot;
    std::shared_ptr<HyperspaceSelector> m_Selector;
    std::shared_ptr<PrivateFrequencyEnforcer> m_PrivEnforcer;
    AnchorSelector m_AnchorSelector;
    bool m_Flagging;

    bool IsFlagging() const {
        return m_Bot->GetFreq() == 90 || m_Bot->GetFreq() == 91;
    }

public:
    HyperspacePlugin(api::Bot* bot)
        : m_Bot(bot),
          m_Selector(new HyperspaceSelector()),
          m_PrivEnforcer(new PrivateFrequencyEnforcer(m_Bot)),
          m_Flagging(false),
          m_AnchorSelector(m_Bot)
    {
    }

    bool IsFlaggingEnabled() const { return m_Flagging; }

    void StartFlagging() {
        ClientPtr client = m_Bot->GetClient();

        m_Flagging = true;

        client->ReleaseKeys();
        client->SetXRadar(false);
        while (!m_Bot->FullEnergy()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            client->Update(100);
        }
        client->SendString("?flag", false);

        std::this_thread::sleep_for(std::chrono::milliseconds(750));
        m_AnchorSelector.ForceSend();

    }

    void StopFlagging() {
        ClientPtr client = m_Bot->GetClient();

        m_Flagging = false;

        client->ReleaseKeys();
        client->SetXRadar(false);
        while (!m_Bot->FullEnergy()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            client->Update(100);
        }

        client->Spec();

    }

    int OnCreate() {
        m_Bot->GetCommandHandler().RegisterCommand("joinfriends", api::CommandPtr(new JoinFriendsCommand(m_PrivEnforcer)));
        m_Bot->GetCommandHandler().RegisterCommand("flag", api::CommandPtr(new FlagCommand(this)));
        return 0;
    }

    int OnUpdate(unsigned long dt) {
        if (!m_Flagging)
            m_PrivEnforcer->OnUpdate(dt);
        else
            m_AnchorSelector.OnUpdate(dt);

        if (m_Flagging && !IsFlagging() && m_Bot->IsInSafe())
            StartFlagging();

        if (m_Flagging && m_Bot->IsInSafe()) {
            api::PlayerPtr anchor = m_AnchorSelector.GetAnchor();

            if (anchor)
                m_Bot->SetAttachTarget(anchor->GetName());
            else
                m_Bot->SetAttachTarget("");

            auto stateType = m_Bot->GetState()->GetType();
            if (stateType != api::StateType::AttachState)
                m_Bot->SetState(api::StateType::AttachState);
        }

        return 0;
    }

    int OnDestroy() {
        m_Bot->GetCommandHandler().UnregisterCommand("joinfriends");
        m_Bot->GetCommandHandler().UnregisterCommand("flag");
        return 0;
    }

    void OnChatMessage(ChatMessage* mesg) {
        m_AnchorSelector.OnChatMessage(mesg);
    }
};

void FlagCommand::Invoke(api::Bot* bot, const std::string& sender, const std::string& args) {
    ClientPtr client = bot->GetClient();
    bool flagging = !m_Plugin->IsFlaggingEnabled();

    std::cout << "Flagging: " << std::boolalpha << flagging << std::endl;

    if (flagging)
        m_Plugin->StartFlagging();
    else
        m_Plugin->StopFlagging();

}

extern "C" {
    PLUGIN_FUNC Plugin* CreatePlugin(api::Bot* bot) {
        return new HyperspacePlugin(bot);
    }

    PLUGIN_FUNC void DestroyPlugin(Plugin* plugin) {
        delete plugin;
    }

    PLUGIN_FUNC const char* GetPluginName() {
        return "Hyperspace";
    }
}
