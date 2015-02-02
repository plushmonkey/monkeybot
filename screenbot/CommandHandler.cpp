#include "CommandHandler.h"

#include "Bot.h"
#include "Random.h"
#include "Client.h"

#include <thread>
#include <regex>

#define RegisterCommand(cmd, func) m_Commands[cmd] = std::bind(&CommandHandler::func, this, std::placeholders::_1);

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

    std::cout << "switching flagged" << std::endl;

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

    tcout << "Config set." << std::endl;
}

void CommandHandler::HandleMessage(ChatMessage* mesg) {
    std::string line = mesg->GetLine();

    std::regex cmd_regex(R"::(^P\s+(.+)> !(.+)$)::");
    std::sregex_iterator begin(line.begin(), line.end(), cmd_regex);
    std::sregex_iterator end;

    if (begin == end) return;

    std::smatch match = *begin;
    std::string player_name = match[1];
    std::string command_line = match[2];

    std::cout << "Command from " << player_name << " : " << command_line << std::endl;

    size_t pos = command_line.find(" ");
    std::string command(command_line);
    std::string args;

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

    if (!allowed) return;

    std::transform(command.begin(), command.end(), command.begin(), tolower);
    auto cmd = m_Commands.find(command);

    if (cmd != m_Commands.end())
        cmd->second(args);
}

CommandHandler::CommandHandler(Bot* bot) : m_Bot(bot) {
    RegisterCommand("flag", CommandFlag);
    RegisterCommand("freq", CommandFreq);
    RegisterCommand("config", CommandConfig);
    RegisterCommand("taunt", CommandTaunt);
}

CommandHandler::~CommandHandler() {

}
