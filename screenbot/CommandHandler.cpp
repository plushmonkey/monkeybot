#include "CommandHandler.h"

#include "Bot.h"
#include "Random.h"
#include "Client.h"
#include "Util.h"
#include "Commands/Commands.h"
#include "Tokenizer.h"

#include <thread>
#include <regex>
#include <sstream>

void CommandHandler::AddPermission(const std::string& player, const std::string& permission) {
    std::string player_lower = Util::strtolower(player);
    std::string perm_lower = Util::strtolower(permission);

    m_Permissions[player_lower].push_back(perm_lower);
}

bool CommandHandler::ComparePermissions(const std::string& has, const std::string& required) {
    using namespace Util;
    using namespace std;

    Tokenizer required_tokens(required);
    Tokenizer has_tokens(has);

    required_tokens('.');
    has_tokens('.');

    size_t has_count = has_tokens.size();
    size_t required_count = required_tokens.size();

    for (size_t i = 0; i < has_count && i < required_count; ++i) {
        if (has_tokens[i].compare("*") == 0) return true;
        if (has_tokens[i].compare(required_tokens[i]) != 0) return false;
    }

    return has_count == required_count;
}

bool CommandHandler::HasPermission(const std::string& player, api::CommandPtr command) {
    std::string cmd_perm = Util::strtolower(command->GetPermission());
    std::string player_lower = Util::strtolower(player);

    if (cmd_perm.size() == 0) return true;

    auto permkv = m_Permissions.find(player_lower);

    // Use wildstar if the player doesn't have any permissions
    if (permkv == m_Permissions.end())
        permkv = m_Permissions.find("*");

    // Player doesn't have any permissions and the command requires permission
    if (permkv == m_Permissions.end()) return false;

    Permissions::const_iterator iter = permkv->second.begin();
    while (iter != permkv->second.end()) {
        if (ComparePermissions(*iter, cmd_perm)) return true;
        ++iter;
    }
    return false;
}

void CommandHandler::HandleMessage(ChatMessage* mesg) {
    if (mesg->GetType() == ChatMessage::Type::Other) return;

    std::string player_name = mesg->GetPlayer();
    std::string command_line = mesg->GetMessage();

    if (command_line.size() <= 1 || command_line.at(0) != '!') return;

    command_line = command_line.substr(1);

    std::string command(command_line);
    std::string args;

    size_t pos = command_line.find(" ");

    if (pos != std::string::npos) {
        command = command_line.substr(0, pos);
        args = command_line.substr(pos + 1);
    }

    std::transform(command.begin(), command.end(), command.begin(), tolower);

    /* Don't allow help in pub chat since it's used for a bunch of bots */
    if (command.compare("help") == 0 && mesg->GetType() == ChatMessage::Type::Public) return;

    std::cout << "Command from " << player_name << " : " << command_line << std::endl;

    auto cmd = m_Commands.find(command);

    if (cmd != m_Commands.end()) {
        if (HasPermission(player_name, cmd->second))
            cmd->second->Invoke(m_Bot, mesg->GetPlayer(), args);
        else
            std::cout << mesg->GetPlayer() << " tried to use command " << command_line << " but doesn't have permission" << std::endl;
    } else {
        std::cout << "Command " << command << " not recognized." << std::endl;
    }
}

bool CommandHandler::RegisterCommand(const std::string& name, api::CommandPtr command) {
    std::string name_lower = Util::strtolower(name);

    auto iter = m_Commands.find(name_lower);
    if (iter != m_Commands.end()) return false;

    m_Commands[name_lower] = command;

    return true;
}

void CommandHandler::UnregisterCommand(const std::string& name) {
    std::string name_lower = Util::strtolower(name);

    auto iter = m_Commands.find(name_lower);
    if (iter == m_Commands.end()) return;

    m_Commands.erase(name_lower);
}

CommandHandler::const_iterator CommandHandler::begin() const {
    return m_Commands.begin();
}

CommandHandler::const_iterator CommandHandler::end() const {
    return m_Commands.end();
}

std::size_t CommandHandler::GetSize() const {
    return m_Commands.size();
}

void CommandHandler::InitPermissions() {
    PermissionMap perms = m_Bot->GetConfig().Permissions;

    m_Permissions.clear();

    // Use AddPermission to make sure everything is lowercase
    for (auto kv : perms) {
        for (auto perm : kv.second)
            AddPermission(kv.first, perm);
    }
}

CommandHandler::CommandHandler(Bot* bot) : m_Bot(bot) {
    RegisterCommand("help", std::make_shared<CommandsCommand>());
    RegisterCommand("commands", std::make_shared<CommandsCommand>());
    RegisterCommand("version", std::make_shared<VersionCommand>());
    RegisterCommand("owner", std::make_shared<OwnerCommand>());

    RegisterCommand("reloadconf", std::make_shared<ReloadConfCommand>());
    RegisterCommand("pause", std::make_shared<PauseCommand>());
    RegisterCommand("say", std::make_shared<SayCommand>());
    
    RegisterCommand("plugins", std::make_shared<PluginsCommand>());
    RegisterCommand("load", std::make_shared<LoadCommand>());
    RegisterCommand("unload", std::make_shared<UnloadCommand>());

    RegisterCommand("ship", std::make_shared<ShipCommand>());
    RegisterCommand("flag", std::make_shared<FlagCommand>());
    RegisterCommand("freq", std::make_shared<FreqCommand>());
    RegisterCommand("target", std::make_shared<TargetCommand>());

    /* Probably should change the name of this and make it part of survivor plugin. */
    RegisterCommand("priority", std::make_shared<PriorityCommand>());

    /* TODO: Pull into own plugins */
    RegisterCommand("commander", std::make_shared<CommanderCommand>());
    RegisterCommand("revenge", std::make_shared<RevengeCommand>());
    RegisterCommand("taunt", std::make_shared<TauntCommand>());
}

CommandHandler::~CommandHandler() {

}
