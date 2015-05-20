#include "PluginInstance.h"

PluginInstance::PluginInstance(const std::string& filename, const std::string& name)
    : m_Filename(filename),
      m_Name(name),
      m_Handle(nullptr),
      m_CreateFunc(nullptr),
      m_Plugin(nullptr)
{
}

PluginInstance::PluginInstance(std::string&& filename, std::string&& name)
    : m_Filename(std::move(filename)),
      m_Name(std::move(name)),
      m_Handle(nullptr),
      m_CreateFunc(nullptr),
      m_Plugin(nullptr)
{
}

PluginInstance::~PluginInstance() {
    if (m_Plugin) {
        PluginDestroyFunc destroy_func = reinterpret_cast<PluginDestroyFunc>(GetProcAddress(m_Handle, "DestroyPlugin"));

        if (destroy_func)
            destroy_func(m_Plugin);
    }

    if (m_Handle)
        FreeLibrary(m_Handle);
}

const std::string& PluginInstance::GetName() const {
    return m_Name;
}

Plugin* PluginInstance::GetPlugin() {
    return m_Plugin;
}

Plugin* PluginInstance::Create(api::Bot* bot) {
    if (!m_Handle)
        m_Handle = LoadLibrary(m_Filename.c_str());

    if (m_Handle) {
        if (!m_CreateFunc)
            m_CreateFunc = reinterpret_cast<PluginCreateFunc>(GetProcAddress(m_Handle, "CreatePlugin"));

        if (m_CreateFunc) {
            m_Plugin = m_CreateFunc(bot);

            if (m_Plugin)
                m_Plugin->OnCreate();

            return m_Plugin;
        }
    }
    return nullptr;
}
