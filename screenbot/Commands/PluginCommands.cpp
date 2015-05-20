#include "PluginCommands.h"

#include "../api/Api.h"
#include "../plugin/PluginManager.h"

#include <iostream>

std::string LoadCommand::GetPermission() {
    return "default.load";
}

void LoadCommand::Invoke(api::Bot* bot, const std::string& sender, const std::string& args) {
    if (args.length() == 0) return;

    PluginManager& pm = bot->GetPluginManager();

    pm.LoadPlugin(bot, args);
}

std::string UnloadCommand::GetPermission() {
    return "default.load";
}

void UnloadCommand::Invoke(api::Bot* bot, const std::string& sender, const std::string& args) {
    if (args.length() == 0) return;

    PluginManager& pm = bot->GetPluginManager();

    pm.UnloadPlugin(args);
}
