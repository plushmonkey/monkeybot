#ifndef COMMANDS_H_
#define COMMANDS_H_

#include "PauseCommand.h"
#include "CommandsCommand.h"
#include "PluginCommands.h"


class ReloadConfCommand : public api::Command {
public:
    std::string GetPermission();
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};


class ShipCommand : public api::Command {
public:
    std::string GetPermission();
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class TargetCommand : public api::Command {
public:
    std::string GetPermission();
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class PriorityCommand : public api::Command {
public:
    std::string GetPermission();
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class TauntCommand : public api::Command {
public:
    std::string GetPermission();
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class SayCommand : public api::Command {
public:
    std::string GetPermission();
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class FreqCommand : public api::Command {
public:
    std::string GetPermission();
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class FlagCommand : public api::Command {
public:
    std::string GetPermission();
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class VersionCommand : public api::Command {
public:
    std::string GetPermission();
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class OwnerCommand : public api::Command {
public:
    std::string GetPermission();
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class CommanderCommand : public api::Command {
public:
    std::string GetPermission();
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};

class RevengeCommand : public api::Command {
public:
    std::string GetPermission();
    void Invoke(api::Bot* bot, const std::string& sender, const std::string& args);
};



#endif
