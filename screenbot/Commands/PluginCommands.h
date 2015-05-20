#ifndef COMMANDS_PLUGIN_COMMANDS_H_
#define COMMANDS_PLUGIN_COMMANDS_H_

#include "../api/Command.h"

class LoadCommand : public api::Command {
public:
    std::string GetPermission();
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class UnloadCommand : public api::Command {
public:
    std::string GetPermission();
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

#endif
