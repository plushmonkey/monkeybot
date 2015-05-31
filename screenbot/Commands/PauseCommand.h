#ifndef COMMANDS_PAUSE_COMMAND_H_
#define COMMANDS_PAUSE_COMMAND_H_

#include "../api/Command.h"

class PauseCommand : public api::Command {
public:
    std::string GetHelp() const { return "Pauses the bot."; }
    std::string GetPermission() const;
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

#endif
