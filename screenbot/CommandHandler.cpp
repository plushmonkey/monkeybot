#include "CommandHandler.h"

#include "Bot.h"
#include "Random.h"
#include "Client.h"

#include <thread>
#include <regex>

#define RegisterCommand(cmd, func) m_Commands[cmd] = std::bind(&CommandHandler::func, this, std::placeholders::_1);

void CommandHandler::CommandTarget(const std::string& args) {
    m_Bot->GetClient()->SetTarget(args);

    std::cout << "Target: " << (args.length() > 0 ? args : "None") << std::endl;
}

void CommandHandler::CommandTaunt(const std::string& args) {
    bool taunt = !m_Bot->GetTaunt();

    m_Bot->SetTaunt(taunt);

    std::cout << "Taunt: " << std::boolalpha << taunt << std::endl;

    if (taunt)
        m_Bot->GetConfig().Set("Taunt", "True");
    else
        m_Bot->GetConfig().Set("Taunt", "False");
}

void CommandHandler::CommandFreq(const std::string& args) {
    ClientPtr client = m_Bot->GetClient();
    int freq = 0;
    
    if (args.length() > 0)
        freq = atoi(args.c_str());
    else
        freq = Random::GetU32(10, 80);

    client->ReleaseKeys();
    client->SetXRadar(false);
    while (client->GetEnergy() < m_Bot->GetMaxEnergy()) {
        client->Update(100);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    client->SendString("=" + std::to_string(freq));
}

void CommandHandler::CommandFlag(const std::string& args) {
    bool flagging = m_Bot->GetFlagging();
    Config& cfg = m_Bot->GetConfig();
    ClientPtr client = m_Bot->GetClient();

    if (!cfg.Get<bool>("Hyperspace")) return;

    std::cout << "Flagging: " << std::boolalpha << !flagging << std::endl;

    if (flagging) {
        m_Bot->SetAttaching(false);
        m_Bot->SetCenterOnly(true);
        
        cfg.Set("Attach", "False");
        cfg.Set("OnlyCenter", "True");

        flagging = false;

        m_Bot->SetCenterRadius(250);

        int freq = Random::GetU32(10, 80);
        client->ReleaseKeys();
        client->SetXRadar(false);
        while (client->GetEnergy() < m_Bot->GetMaxEnergy()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            client->Update(100);
        }
        client->SendString("=" + std::to_string(freq));
    } else {
        m_Bot->SetAttaching(true);
        m_Bot->SetCenterOnly(false);

        cfg.Set("Attach", "True");
        cfg.Set("OnlyCenter", "False");

        flagging = true;

        m_Bot->SetCenterRadius(200);

        int current_freq = client->GetFreq();

        client->ReleaseKeys();
        client->SetXRadar(false);
        while (client->GetEnergy() < m_Bot->GetMaxEnergy()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            client->Update(100);
        }
        client->SendString("?flag");
    }

    m_Bot->SetFlagging(flagging);
}

void CommandHandler::CommandConfig(const std::string& args) {
    std::size_t pos = args.find(" ");

    if (pos == std::string::npos) {
        tcerr << "Error with config command" << std::endl;
        return;
    }

    std::string variable = args.substr(0, pos);
    std::string value = args.substr(pos + 1);

    m_Bot->GetConfig().Set(variable, value);

    m_Bot->ReloadConfig();

    tcout << variable << " = " << value  << std::endl;
}

void CommandHandler::CommandPause(const std::string& args) {
    bool paused = !m_Bot->GetPaused();

    m_Bot->SetPaused(paused);
    tcout << "Paused: " << std::boolalpha << paused << std::endl;

    if (paused)
        m_Bot->GetClient()->ReleaseKeys();
}

void CommandHandler::HandleMessage(ChatMessage* mesg) {
    if (mesg->GetType() != ChatMessage::Type::Private && mesg->GetType() != ChatMessage::Type::Channel) return;

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

    static const std::vector<std::string> staff = { "monkey", "ceiu", "bzap", "baked cake", "cdb-man", "nn" };
    std::transform(player_name.begin(), player_name.end(), player_name.begin(), tolower);
    bool allowed = false;
    for (auto s : staff) {
        if (player_name.compare(s) == 0) {
            allowed = true;
            break;
        }
    }

    if (player_name.compare(m_Owner) == 0)
        allowed = true;

    if (!allowed) {
        std::cout << player_name << " tried to use command " << command_line << " but doesn't have permission" << std::endl;
        return;
    }

    std::cout << "Command from " << player_name << " : " << command_line << std::endl;

    std::transform(command.begin(), command.end(), command.begin(), tolower);
    auto cmd = m_Commands.find(command);

    if (cmd != m_Commands.end())
        cmd->second(args);
    else
        std::cout << "Command " << command << " not recognized." << std::endl;
}

bool CommandHandler::Initialize() {
    m_Owner = m_Bot->GetConfig().Get<std::string>("Owner");
    std::transform(m_Owner.begin(), m_Owner.end(), m_Owner.begin(), tolower);
    return true;
}

CommandHandler::CommandHandler(Bot* bot) : m_Bot(bot) {
    RegisterCommand("flag", CommandFlag);
    RegisterCommand("freq", CommandFreq);
    RegisterCommand("config", CommandConfig);
    RegisterCommand("taunt", CommandTaunt);
    RegisterCommand("target", CommandTarget);
    RegisterCommand("pause", CommandPause);
}

CommandHandler::~CommandHandler() {

}
