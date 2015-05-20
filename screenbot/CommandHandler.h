#ifndef COMMAND_HANDLER_H_
#define COMMAND_HANDLER_H_

#include "Common.h"
#include "Messages/Messages.h"

#include <functional>
#include <map>

class Bot;

class CommandHandler : public api::CommandHandler, public MessageHandler<ChatMessage> {
private:
    typedef std::vector<std::string> Permissions;
    typedef std::map<std::string, Permissions> PermissionMap;

    PermissionMap m_Permissions;
    Commands m_Commands;
    Bot* m_Bot;

    // Determine if first falls under second
    bool ComparePermissions(const std::string& has, const std::string& required);
    void HandleMessage(ChatMessage* mesg);
public:
    CommandHandler(Bot* bot);
    ~CommandHandler();

    void AddPermission(const std::string& player, const std::string& permission);
    bool HasPermission(const std::string& player, api::CommandPtr command);

    bool RegisterCommand(const std::string& name, api::CommandPtr command);
    void UnregisterCommand(const std::string& name);

    void InitPermissions();

    const_iterator begin() const;
    const_iterator end() const;
    std::size_t GetSize() const;
};

#endif
