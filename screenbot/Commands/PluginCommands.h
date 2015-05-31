#ifndef COMMANDS_PLUGIN_COMMANDS_H_
#define COMMANDS_PLUGIN_COMMANDS_H_

#include "../api/Command.h"

class PluginsCommand : public api::Command {
public:
    std::string GetHelp() const{ return "Returns a list of plugins that are loaded."; }
    std::string GetPermission() const;
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class LoadCommand : public api::Command {
public:
    std::string GetHelp() const{ return "Loads a plugin from the plugins directory."; }
    std::string GetPermission() const;
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class UnloadCommand : public api::Command {
public:
    std::string GetHelp() const{ return "Unloads a plugin."; }
    std::string GetPermission() const;
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

#endif
