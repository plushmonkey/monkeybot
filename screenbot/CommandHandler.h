#ifndef COMMAND_HANDLER_H_
#define COMMAND_HANDLER_H_

#include "Common.h"
#include "Messages/Messages.h"

#include <functional>
#include <map>

class Bot;

typedef std::function<void(const std::string&, const std::string&)> CommandFunction;

class CommandHandler : public MessageHandler<ChatMessage> {
private:
    std::map<std::string, CommandFunction> m_Commands;
    Bot* m_Bot;

    void HandleMessage(ChatMessage* mesg);

    void CommandShip(const std::string& sender, const std::string& args);
    void CommandSpec(const std::string& sender, const std::string& args);
    void CommandPause(const std::string& sender, const std::string& args);
    void CommandTarget(const std::string& sender, const std::string& args);
    void CommandPriority(const std::string& sender, const std::string& args);
    void CommandFreq(const std::string& sender, const std::string& args);
    void CommandFlag(const std::string& sender, const std::string& args);
    void CommandTaunt(const std::string& sender, const std::string& args);
    void CommandCommander(const std::string& sender, const std::string& args);
    void CommandSay(const std::string& sender, const std::string& args);
    void CommandRevenge(const std::string& sender, const std::string& args);
    void CommandWarp(const std::string& sender, const std::string& args);
    void CommandCommands(const std::string& sender, const std::string& args);
    void CommandLoad(const std::string& sender, const std::string& args);
    void CommandUnload(const std::string& sender, const std::string& args);

public:
    CommandHandler(Bot* bot);
    ~CommandHandler();
};

#endif
