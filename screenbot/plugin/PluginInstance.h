#ifndef PLUGIN_PLUGIN_INSTANCE_H_
#define PLUGIN_PLUGIN_INSTANCE_H_

#include "Plugin.h"

#include <string>
#include <Windows.h>

class PluginInstance {
private:
    std::string m_Filename;
    std::string m_Name;
    HINSTANCE m_Handle;
    PluginCreateFunc m_CreateFunc;
    Plugin* m_Plugin;

    PluginInstance(const PluginInstance& other);
    PluginInstance& operator=(const PluginInstance& other);
public:
    PluginInstance(const std::string& filename, const std::string& name);
    PluginInstance(std::string&& filename, std::string&& name);
    ~PluginInstance();

    const std::string& GetName() const;
    
    Plugin* GetPlugin();
    Plugin* Create(api::Bot* bot);
};

#endif
