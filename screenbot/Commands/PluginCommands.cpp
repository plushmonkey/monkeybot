#include "PluginCommands.h"

#include "../api/Api.h"
#include "../plugin/PluginManager.h"

#include <iostream>
#include <sstream>

std::string PluginsCommand::GetPermission() {
    return "default.plugins";
}

void PluginsCommand::Invoke(api::Bot* bot, const std::string& sender, const std::string& args) {
    PluginManager& pm = bot->GetPluginManager();
    ClientPtr client = bot->GetClient();
    std::stringstream ss;

    ss << "Plugins (" << pm.GetCount() << "): ";

    for (PluginManager::const_iterator iter = pm.begin();
         iter != pm.end();
         ++iter)
    {
        std::string name = (*iter)->GetName();

        if (iter != pm.begin())
            ss << ", ";

        ss << name;

        if (ss.str().length() > 140) {
            client->SendPM(sender, ss.str());
            ss.str("");
        }
    }

    if (ss.str().length() > 0)
        client->SendPM(sender, ss.str());
}

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
