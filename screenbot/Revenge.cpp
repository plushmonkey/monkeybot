#include "Revenge.h"

#include "Bot.h"
#include "Util.h"
#include "Client.h"

#include <algorithm>
#include <functional>

namespace {

const std::string BotName = "taz";
const unsigned int Timeout = 2 * 60 * 1000;

}

Revenge::Revenge(Bot& bot, api::Ship current_ship, api::Ship strong_ship, int max_deaths)
    : m_Bot(bot), m_DeathCounter(0), m_Enabled(false),
    m_NormalShip(current_ship), m_StrongShip(strong_ship),
    m_MaxDeaths(max_deaths), m_LastDeath(0)
{
    if (bot.GetName().compare(BotName) == 0)
        m_Enabled = true;
    m_UpdateID = RegisterBotUpdater(&bot, Revenge::OnUpdate);
}

Revenge::~Revenge() {
    m_Bot.UnregisterUpdater(m_UpdateID);
}

bool Revenge::OnUpdate(api::Bot *bot, unsigned long dt) {
    if (!m_Enabled) return true;

    DWORD current_time = timeGetTime();

    if (m_LastDeath > 0 && m_Bot.GetShip() == m_StrongShip && current_time - m_LastDeath > Timeout)
        Reset();
    return true;
}

void Revenge::SetEnabled(bool enabled) {
    m_Enabled = enabled;

    if (!enabled)
        Reset();
}

void Revenge::Reset() {
    m_LastDeath = 0;
    m_DeathCounter = 0;
    m_Bot.SetShip(m_NormalShip);
}

void Revenge::HandleMessage(KillMessage* mesg) {
    if (!m_Enabled) return;

    std::string killer = Util::strtolower(mesg->GetKiller());
    std::string killed = Util::strtolower(mesg->GetKilled());

    // Don't do anything if suicide
    if (killer.compare(killed) == 0) return;

    if (killer.compare(BotName) == 0 && !IsWhitelisted(killed))
        OnKill(mesg);
    else if (killed.compare(BotName) == 0 && !IsWhitelisted(killer))
        OnDeath(mesg);
}

bool Revenge::IsWhitelisted(const std::string& name) {
    const std::vector<std::string>& whitelist = m_Bot.GetConfig().TauntWhitelist;
    return std::find(whitelist.begin(), whitelist.end(), name) != whitelist.end();
}

void Revenge::OnKill(KillMessage *mesg) {
    api::Ship ship = m_Bot.GetShip();

    m_DeathCounter = std::max(m_DeathCounter - 1, 0);

    if (m_DeathCounter <= 0 && ship == m_StrongShip) {
        m_Bot.SetShip(m_NormalShip);

        m_Bot.GetClient()->SetTarget(m_PrevTarget);
        m_PrevTarget.clear();
    }

    if (ship == m_StrongShip) {
        std::string target = m_Bot.GetClient()->GetTarget();

        // Set the target back to what it was
        if (mesg->GetKilled().compare(target) == 0) {
            m_Bot.GetClient()->SetTarget(m_PrevTarget);
            m_PrevTarget.clear();
        }
    }
}

void Revenge::OnDeath(KillMessage *mesg) {
    if (m_Bot.GetFlagging()) return;

    m_LastDeath = timeGetTime();

    api::Ship ship = m_Bot.GetShip();

    m_DeathCounter = std::min(m_MaxDeaths, m_DeathCounter + 1);

    if (m_DeathCounter >= m_MaxDeaths && ship != m_StrongShip) {
        m_NormalShip = m_Bot.GetShip();
        m_Bot.SetShip(m_StrongShip);
        m_PrevTarget = m_Bot.GetClient()->GetTarget();
        m_Bot.GetClient()->SetTarget(mesg->GetKiller());
    }
}
