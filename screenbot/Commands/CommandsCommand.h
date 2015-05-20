#ifndef COMMANDS_COMMANDS_COMMAND_H_
#define COMMANDS_COMMANDS_COMMAND_H_

#include "../api/Command.h"

class CommandsCommand : public api::Command {
public:
    std::string GetPermission();
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

#endif
