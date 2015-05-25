#include "Commands.h"

#include "../Bot.h"
#include "../Random.h"
#include "../Util.h"
#include "../Selector.h"

#include <thread>


std::string ReloadConfCommand::GetPermission() {
    return "default.reloadconf";
}

void ReloadConfCommand::Invoke(api::Bot* bot, const std::string& sender, const std::string& args) {
    bot->GetConfig().Load("config.json");
    ((CommandHandler&)bot->GetCommandHandler()).InitPermissions();
}

std::string ShipCommand::GetPermission() {
    return "default.ship";
}

void ShipCommand::Invoke(api::Bot* bot, const std::string& sender, const std::string& args) {
    if (args.length() == 0) return;

    int ship = atoi(args.c_str());
    if (ship < 1 || ship > 8) return;

    ClientPtr client = bot->GetClient();
    bot->SetShip((api::Ship)(ship - 1));

    std::cout << "Ship: " << ship << std::endl;
}

std::string TargetCommand::GetPermission() {
    return "default.target";
}

void TargetCommand::Invoke(api::Bot* bot, const std::string& sender, const std::string& args) {
    if (args.length() > 0)
        bot->SetEnemySelector(api::SelectorPtr(new TargetEnemySelector(args)));
    else
        bot->SetEnemySelector(api::SelectorPtr(new ClosestEnemySelector()));

    std::cout << "Target: " << (args.length() > 0 ? args : "None") << std::endl;
}

std::string PriorityCommand::GetPermission() {
    return "default.priority";
}

void PriorityCommand::Invoke(api::Bot* bot, const std::string& sender, const std::string& args) {
    bot->GetClient()->SetPriorityTarget(args);

    std::cout << "Priority Target: " << (args.length() > 0 ? args : "None") << std::endl;

    ((Bot*)bot)->GetSurvivorGame()->SetTarget(args);
}

std::string TauntCommand::GetPermission() {
    return "default.taunt";
}

void TauntCommand::Invoke(api::Bot* bot, const std::string& sender, const std::string& args) {
    bool taunt = !bot->GetConfig().Taunt;

    ((Bot*)bot)->SetTaunt(taunt);

    std::cout << "Taunt: " << std::boolalpha << taunt << std::endl;
}

std::string SayCommand::GetPermission() {
    return "default.say";
}

void SayCommand::Invoke(api::Bot* bot, const std::string& sender, const std::string& args) {
    bot->GetClient()->SendString(args);
}

std::string FreqCommand::GetPermission() {
    return "default.freq";
}

void FreqCommand::Invoke(api::Bot* bot, const std::string& sender, const std::string& args) {
    ClientPtr client = bot->GetClient();
    int freq = 0;

    if (args.length() > 0)
        freq = atoi(args.c_str());
    else
        freq = Random::GetU32(10, 80);

    client->ReleaseKeys();
    client->SetXRadar(false);
    while (!bot->FullEnergy()) {
        client->Update(100);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    client->SendString("=" + std::to_string(freq));
}

std::string FlagCommand::GetPermission() {
    return "default.flag";
}

void FlagCommand::Invoke(api::Bot* bot, const std::string& sender, const std::string& args) {
    bool flagging = ((Bot*)bot)->GetFlagging();
    Config& cfg = bot->GetConfig();
    ClientPtr client = bot->GetClient();

    if (!cfg.Hyperspace) return;

    std::cout << "Flagging: " << std::boolalpha << !flagging << std::endl;

    if (flagging) {
        cfg.Attach = false;
        cfg.CenterOnly = true;

        flagging = false;

        cfg.CenterRadius = 250;

        int freq = Random::GetU32(10, 80);
        client->ReleaseKeys();
        client->SetXRadar(false);
        while (!bot->FullEnergy()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            client->Update(100);
        }
        client->SendString("=" + std::to_string(freq));
    } else {
        cfg.Attach = true;
        cfg.CenterOnly = false;

        flagging = true;

        cfg.CenterRadius = 200;

        client->ReleaseKeys();
        client->SetXRadar(false);
        while (!bot->FullEnergy()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            client->Update(100);
        }
        client->SendString("?flag");
    }

    ((Bot*)bot)->SetFlagging(flagging);
}

std::string VersionCommand::GetPermission() {
    return "";
}

void VersionCommand::Invoke(api::Bot* bot, const std::string& sender, const std::string& args) {
    std::string version = bot->GetVersion().GetString();

    std::string to_send = "monkeybot v" + version;

    bot->GetClient()->SendPM(sender, to_send);
}

std::string OwnerCommand::GetPermission() {
    return "";
}

void OwnerCommand::Invoke(api::Bot* bot, const std::string& sender, const std::string& args) {
    Config& conf = bot->GetConfig();

    std::string owner = conf.Owner;

    bot->GetClient()->SendPM(sender, "Owner: " + owner);
}


std::string CommanderCommand::GetPermission() {
    return "default.commander";
}

void CommanderCommand::Invoke(api::Bot* bot, const std::string& sender, const std::string& args) {
    bot->GetConfig().Commander = !bot->GetConfig().Commander;

    std::cout << "Commander: " << std::boolalpha << bot->GetConfig().Commander << std::endl;
}

std::string RevengeCommand::GetPermission() {
    return "default.revenge";
}

void RevengeCommand::Invoke(api::Bot* bot, const std::string& sender, const std::string& args) {
    bool enabled = false;

    if (args.length() == 0)
        enabled = !((Bot*)bot)->GetRevenge()->IsEnabled();
    else
        enabled = Util::strtobool(args);

    ((Bot*)bot)->GetRevenge()->SetEnabled(enabled);

    std::cout << "Revenge: " << enabled << std::endl;
}