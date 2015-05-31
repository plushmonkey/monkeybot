#include "CommandsCommand.h"

#include "../api/Api.h"
#include "../Bot.h"

#include <iostream>
#include <sstream>

std::string CommandsCommand::GetPermission() const {
    return "";
}

void CommandsCommand::Invoke(api::Bot* bot, const std::string& sender, const std::string& args) {
    std::stringstream ss;
    std::size_t size = 0;
    ClientPtr client = bot->GetClient();
    api::CommandHandler& cmd_handler = bot->GetCommandHandler();

    std::size_t num_commands = cmd_handler.GetSize();
    std::size_t i = 0;

    ss << "Commands: ";

    for (const auto& kv : cmd_handler) {
        std::string required_permission = kv.second->GetPermission();
        std::string command = kv.first;

        if (!cmd_handler.HasPermission(sender, kv.second)) continue;

        if (++i != 1 && size != 0)
            ss << ", ";

        ss << command;

        size += command.length() + 2;

        if (size > 140) {
            client->SendPM(sender, ss.str());
            ss.str("");
            size = 0;
        }
    }

    if (size > 0 && ss.str().compare("Commands: ") != 0)
        client->SendPM(sender, ss.str());
}
