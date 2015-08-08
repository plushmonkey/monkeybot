#ifndef PLUGIN_PLUGIN_MANAGER_H_
#define PLUGIN_PLUGIN_MANAGER_H_

#include "PluginInstance.h"
#include "../Messages/Messages.h"

#include <vector>
#include <string>
#include <map>

class Bot;

class PluginManager : public MessageHandler<ChatMessage>, 
                      public MessageHandler<KillMessage>,
                      public MessageHandler<EnterMessage>,
                      public MessageHandler<LeaveMessage> {
public:
    typedef std::vector<PluginInstance*> Plugins;
    typedef Plugins::const_iterator const_iterator;
    typedef Plugins::const_reference const_reference;

private:
    Plugins m_Plugins;

    void ClearPlugins();

    void HandleMessage(ChatMessage* mesg);
    void HandleMessage(KillMessage* mesg);
    void HandleMessage(EnterMessage* mesg);
    void HandleMessage(LeaveMessage* mesg);
    std::string PluginManager::GetPluginPath(const std::string& name);
public:
    PluginManager();
    ~PluginManager();

    bool OnUpdate(api::Bot* bot, unsigned long dt);
    int LoadPlugins(Bot* bot, const std::string& directory);

    // Loads a single plugin. Prepends plugins/ and appends .dll
    bool LoadPlugin(api::Bot* bot, const std::string& name);
    void UnloadPlugin(const std::string& name);

    Plugins::size_type GetCount() const;

    const_iterator begin() const;
    const_iterator end() const;
    const_reference operator[](Plugins::size_type index) const;
};

#endif
