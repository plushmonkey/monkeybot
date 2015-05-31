#include "PauseCommand.h"

#include "../api/Api.h"

#include <iostream>

std::string PauseCommand::GetPermission() const {
    return "default.pause";
}

void PauseCommand::Invoke(api::Bot* bot, const std::string& sender, const std::string& args) {
    bool paused = !bot->GetPaused();

    static api::Ship prev_ship = bot->GetShip();

    bot->SetPaused(paused);
    std::cout << "Paused: " << std::boolalpha << paused << std::endl;

    if (paused) {
        prev_ship = bot->GetShip();
        bot->GetClient()->ReleaseKeys();
        bot->SetShip(api::Ship::Spectator);
    } else {
        bot->SetShip(prev_ship);
    }
}
