#ifndef COMMANDS_COMMANDS_COMMAND_H_
#define COMMANDS_COMMANDS_COMMAND_H_

#include "../api/Command.h"

class CommandsCommand : public api::Command {
public:
    std::string GetHelp() const { return "Returns a list of commands. Only returns the commands that the requester has access to."; }
    std::string GetPermission() const;
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

#endif
