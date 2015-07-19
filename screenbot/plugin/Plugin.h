#ifndef PLUGIN_PLUGIN_H_
#define PLUGIN_PLUGIN_H_

#include "../Messages/Messages.h"

class Plugin {
private:
    Plugin(const Plugin& other);
    Plugin& operator=(const Plugin& other);

public:
    Plugin() { }
    virtual ~Plugin() { }

    virtual int OnCreate() = 0;
    virtual int OnUpdate(unsigned long dt) = 0;
    virtual int OnDestroy() = 0;

    virtual void OnChatMessage(ChatMessage* mesg) { }
    virtual void OnKill(KillMessage* mesg) { }
    virtual void OnEnter(EnterMessage* mesg) { }
    virtual void OnLeave(LeaveMessage* mesg) { }
};

namespace api { class Bot; }
typedef Plugin*(*PluginCreateFunc)(api::Bot*);
typedef void(*PluginDestroyFunc)(Plugin*);

typedef const char*(*PluginNameFunc)();

#define PLUGIN_FUNC __declspec(dllexport)

#endif
