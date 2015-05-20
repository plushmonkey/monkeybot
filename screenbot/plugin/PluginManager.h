#ifndef PLUGIN_PLUGIN_MANAGER_H_
#define PLUGIN_PLUGIN_MANAGER_H_

#include "PluginInstance.h"
#include "../Messages/Messages.h"

#include <vector>
#include <string>
#include <map>

class Bot;

class PluginManager : public MessageHandler<ChatMessage> {
public:
    typedef std::vector<PluginInstance*> Plugins;
    typedef Plugins::const_iterator const_iterator;
    typedef Plugins::const_reference const_reference;

private:
    Plugins m_Plugins;
    unsigned int m_UpdateID;

    void ClearPlugins();

    bool OnUpdate(Bot* bot, unsigned long dt);
    void HandleMessage(ChatMessage* mesg);
    std::string PluginManager::GetPluginPath(const std::string& name);
public:
    PluginManager();
    ~PluginManager();

    int LoadPlugins(Bot* bot, const std::string& directory);

    // Loads a single plugin. Prepends plugins/ and appends .dll
    void LoadPlugin(Bot* bot, const std::string& name);
    void UnloadPlugin(const std::string& name);

    Plugins::size_type GetCount() const;

    const_iterator begin() const;
    const_iterator end() const;
    const_reference operator[](Plugins::size_type index) const;
};

#endif
