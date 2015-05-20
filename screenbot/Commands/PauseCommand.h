#ifndef COMMANDS_PAUSE_COMMAND_H_
#define COMMANDS_PAUSE_COMMAND_H_

#include "../api/Command.h"

class PauseCommand : public api::Command {
public:
    std::string GetPermission();
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

#endif
