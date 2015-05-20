#include "PauseCommand.h"

#include "../api/Api.h"

#include <iostream>

std::string PauseCommand::GetPermission() {
    return "default.pause";
}

void PauseCommand::Invoke(api::Bot* bot, const std::string& sender, const std::string& args) {
    bool paused = !bot->GetPaused();

    bot->SetPaused(paused);
    std::cout << "Paused: " << std::boolalpha << paused << std::endl;

    if (paused) {
        bot->GetClient()->ReleaseKeys();
        bot->SetShip(api::Ship::Spectator);
    }
}
