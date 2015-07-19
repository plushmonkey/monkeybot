#include "PluginManager.h"
#include "../Bot.h"
#include "../Util.h"

void PluginManager::ClearPlugins() {
    for (Plugins::size_type i = 0; i < m_Plugins.size(); ++i)
        delete m_Plugins[i];
    m_Plugins.clear();
}

PluginManager::PluginManager() 
{
}
PluginManager::~PluginManager() {
    ClearPlugins();
}

std::string PluginManager::GetPluginPath(const std::string& name) {
    std::string filename;

    std::size_t pos = name.find("plugins\\");
    if (pos == std::string::npos)
        pos = name.find("plugins/");

    if (pos != std::string::npos)
        filename = name.substr(pos + strlen("plugins/"));

    if (filename.length() == 0)
        filename = name;

    pos = filename.find(".dll");
    if (pos != std::string::npos)
        filename = filename.substr(0, pos);

    filename = "plugins\\" + filename + ".dll";

    return filename;
}

void PluginManager::LoadPlugin(api::Bot* bot, const std::string& name) {
    if (name.length() == 0) return;

    std::string filename = GetPluginPath(name);
    
    HINSTANCE dll_handle = LoadLibrary(filename.c_str());

    if (!dll_handle) return;

    PluginCreateFunc create_func = reinterpret_cast<PluginCreateFunc>(GetProcAddress(dll_handle, "CreatePlugin"));
    PluginNameFunc name_func = reinterpret_cast<PluginNameFunc>(GetProcAddress(dll_handle, "GetPluginName"));

    if (create_func && name_func) {
        PluginInstance* plugin = new PluginInstance(filename, name_func());

        if (plugin) {
            m_Plugins.push_back(plugin);
            plugin->Create(bot);

            std::cout << plugin->GetName() << " loaded." << std::endl;
        }
    }
    FreeLibrary(dll_handle);
}

void PluginManager::UnloadPlugin(const std::string& name) {
    std::string find = Util::strtolower(name);

    for (Plugins::size_type i = 0; i < m_Plugins.size(); ++i) {
        std::string plugin_name = Util::strtolower(m_Plugins[i]->GetName());

        if (find.compare(plugin_name) == 0) {
            PluginInstance* inst = m_Plugins[i];

            inst->GetPlugin()->OnDestroy();

            std::cout << plugin_name << " unloaded." << std::endl;
            m_Plugins.erase(m_Plugins.begin() + i);

            delete inst;
        }
    }
}

int PluginManager::LoadPlugins(Bot* bot, const std::string& directory) {
    if (directory.length() == 0) return 0;

    char last_char = directory.at(directory.length() - 1);

    std::string find = directory;

    bool trailing = (last_char == '/' || last_char == '\\');

    if (trailing)
        find.append("*.dll");
    else
        find.append("\\*.dll");

    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(find.c_str(), &fd);

    if (hFind == INVALID_HANDLE_VALUE) {
        FindClose(hFind);
        return 0;
    }

    int count = 0;

    /* Load all the plugins in the directory */
    do {
        HINSTANCE dll_handle = nullptr;
        try {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::string path = directory;
                if (!trailing) path += "\\";
                path.append(fd.cFileName);

                if ((dll_handle = LoadLibrary(path.c_str()))) {
                    PluginCreateFunc create_func = reinterpret_cast<PluginCreateFunc>(GetProcAddress(dll_handle, "CreatePlugin"));
                    PluginNameFunc name_func = reinterpret_cast<PluginNameFunc>(GetProcAddress(dll_handle, "GetPluginName"));

                    if (create_func && name_func) {
                        PluginInstance* plugin = new PluginInstance(path, name_func());

                        if (plugin) {
                            m_Plugins.push_back(plugin);
                            plugin->Create(bot);
                            ++count;
                        }
                    }
                    FreeLibrary(dll_handle);
                }
            }
        } catch (std::exception&) {
            if (dll_handle) FreeLibrary(dll_handle);
        }
    } while (FindNextFile(hFind, &fd));

    FindClose(hFind);

    return count;
}

PluginManager::Plugins::size_type PluginManager::GetCount() const {
    return m_Plugins.size();
}
PluginManager::const_iterator PluginManager::begin() const {
    return m_Plugins.begin();
}
PluginManager::const_iterator PluginManager::end() const {
    return m_Plugins.end();
}
PluginManager::const_reference PluginManager::operator[](Plugins::size_type index) const {
    return m_Plugins[index];
}

bool PluginManager::OnUpdate(api::Bot* bot, unsigned long dt) {
    auto iter = begin();

    while (iter != end()) {
        Plugin* plugin = (*iter)->GetPlugin();
        if (plugin)
            plugin->OnUpdate(dt);
        ++iter;
    }
    return true;
}

void PluginManager::HandleMessage(ChatMessage* mesg) {
    auto iter = begin();

    while (iter != end()) {
        Plugin* plugin = (*iter)->GetPlugin();
        if (plugin)
            plugin->OnChatMessage(mesg);
        ++iter;
    }
}

void PluginManager::HandleMessage(KillMessage* mesg) {
    auto iter = begin();

    while (iter != end()) {
        Plugin* plugin = (*iter)->GetPlugin();
        if (plugin)
            plugin->OnKill(mesg);
        ++iter;
    }
}

void PluginManager::HandleMessage(EnterMessage* mesg) {
    auto iter = begin();

    while (iter != end()) {
        Plugin* plugin = (*iter)->GetPlugin();
        if (plugin)
            plugin->OnEnter(mesg);
        ++iter;
    }
}

void PluginManager::HandleMessage(LeaveMessage* mesg) {
    auto iter = begin();

    while (iter != end()) {
        Plugin* plugin = (*iter)->GetPlugin();
        if (plugin)
            plugin->OnLeave(mesg);
        ++iter;
    }
}