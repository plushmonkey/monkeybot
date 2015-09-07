#include "ShipEnforcer.h"

#include "Bot.h"

bool ShipEnforcer::OnUpdate(api::Bot* bot, unsigned long dt) {
    api::Ship real_ship = bot->GetMemorySensor().GetBotPlayer()->GetShip();
    api::Ship bot_ship = bot->GetShip();

    if (bot_ship == real_ship)
        return true;
    
    bot->SetShip(bot_ship);
    
    return true;
}

ShipEnforcer::ShipEnforcer(api::Bot* bot)
    : m_Bot(bot)
{
    m_UpdateID = bot->RegisterUpdater(std::bind(&ShipEnforcer::OnUpdate, this, std::placeholders::_1, std::placeholders::_2));
}

ShipEnforcer::~ShipEnforcer() {
    m_Bot->UnregisterUpdater(m_UpdateID);
}
