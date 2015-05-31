#ifndef COMMANDS_H_
#define COMMANDS_H_

#include "PauseCommand.h"
#include "CommandsCommand.h"
#include "PluginCommands.h"

class HelpCommand : public api::Command {
public:
    std::string GetHelp() const { return "Helps stuff do stuff and things."; }
    std::string GetPermission() const;
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class ReloadConfCommand : public api::Command {
public:
    std::string GetHelp() const { return "Reloads the config file. Not fully implemented yet."; }
    std::string GetPermission() const;
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class ShipCommand : public api::Command {
public:
    std::string GetHelp() const { return "Sets the current ship. (1-8)"; }
    std::string GetPermission() const;
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class TargetCommand : public api::Command {
public:
    std::string GetHelp() const { return "Makes the bot target a certain player."; }
    std::string GetPermission() const;
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class TauntCommand : public api::Command {
public:
    std::string GetHelp() const { return "Turns on taunt-on-kill. Set the taunts in config.json."; }
    std::string GetPermission() const;
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class SayCommand : public api::Command {
public:
    std::string GetHelp() const { return "Makes the bot type a message."; }
    std::string GetPermission() const;
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class FreqCommand : public api::Command {
public:
    std::string GetHelp() const { return "Sets the bot's frequency."; }
    std::string GetPermission() const;
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class FlagCommand : public api::Command {
public:
    std::string GetHelp() const { return "Makes the bot type ?flag and sets certain config options to make it work for flagging in hyperspace."; }
    std::string GetPermission() const;
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class VersionCommand : public api::Command {
public:
    std::string GetHelp() const { return "Returns the version of monkeybot."; }
    std::string GetPermission() const;
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class OwnerCommand : public api::Command {
public:
    std::string GetHelp() const { return "Returns the owner of the bot. Set in config.json."; }
    std::string GetPermission() const;
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class CommanderCommand : public api::Command {
public:
    std::string GetHelp() const { return "Toggles commander mode for the bot. Makes the bot send its target to first chat."; }
    std::string GetPermission() const;
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class RevengeCommand : public api::Command {
public:
    std::string GetHelp() const { return "Toggles revenge mode. Revenge mode makes the bot switch to a different ship after it dies too many times."; }
    std::string GetPermission() const;
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

#endif
