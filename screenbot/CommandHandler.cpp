#include "CommandHandler.h"

#include "Bot.h"
#include "Random.h"
#include "Client.h"

#include <thread>
#include <regex>

#define RegisterCommand(cmd, func) m_Commands[cmd] = std::bind(&CommandHandler::func, this, std::placeholders::_1);

void CommandHandler::CommandShip(const std::string& args) {
    if (args.length() == 0) return;

    int ship = atoi(args.c_str());
    if (ship < 1 || ship > 8) return;

    ClientPtr client = m_Bot->GetClient();
    m_Bot->SetShip((Ship)(ship - 1));

    std::cout << "Ship: " << ship << std::endl;
}

void CommandHandler::CommandTarget(const std::string& args) {
    m_Bot->GetClient()->SetTarget(args);

    std::cout << "Target: " << (args.length() > 0 ? args : "None") << std::endl;
}

void CommandHandler::CommandPriority(const std::string& args) {
    m_Bot->GetClient()->SetPriorityTarget(args);

    std::cout << "Priority Target: " << (args.length() > 0 ? args : "None") << std::endl;

    m_Bot->GetSurvivorGame()->SetTarget(args);
}

void CommandHandler::CommandTaunt(const std::string& args) {
    bool taunt = !m_Bot->GetConfig().Taunt;

    m_Bot->SetTaunt(taunt);

    std::cout << "Taunt: " << std::boolalpha << taunt << std::endl;
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
    while (!m_Bot->FullEnergy()) {
        client->Update(100);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    client->SendString("=" + std::to_string(freq));
}

void CommandHandler::CommandFlag(const std::string& args) {
    bool flagging = m_Bot->GetFlagging();
    Config& cfg = m_Bot->GetConfig();
    ClientPtr client = m_Bot->GetClient();

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
        while (!m_Bot->FullEnergy()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            client->Update(100);
        }
        client->SendString("=" + std::to_string(freq));
    } else {
        cfg.Attach = true;
        cfg.CenterOnly = false;

        flagging = true;

        cfg.CenterRadius = 200;

        int current_freq = client->GetFreq();

        client->ReleaseKeys();
        client->SetXRadar(false);
        while (!m_Bot->FullEnergy()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            client->Update(100);
        }
        client->SendString("?flag");
    }

    m_Bot->SetFlagging(flagging);
}

void CommandHandler::CommandPause(const std::string& args) {
    bool paused = !m_Bot->GetPaused();

    m_Bot->SetPaused(paused);
    tcout << "Paused: " << std::boolalpha << paused << std::endl;

    if (paused)
        m_Bot->GetClient()->ReleaseKeys();
}

void CommandHandler::CommandCommander(const std::string& args) {
    m_Bot->GetConfig().Commander = !m_Bot->GetConfig().Commander;

    std::cout << "Commander: " << std::boolalpha << m_Bot->GetConfig().Commander << std::endl;
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

    std::vector<std::string> staff = { "monkey", "ceiu", "bzap", "baked cake", "cdb-man", "nn", "pity.", "noldec" };

    /* Append config staff list to hardcoded staff list */
    staff.insert(staff.end(), m_Bot->GetConfig().Staff.begin(), m_Bot->GetConfig().Staff.end());

    std::transform(player_name.begin(), player_name.end(), player_name.begin(), tolower);
    bool allowed = false;
    for (auto s : staff) {
        if (player_name.compare(s) == 0) {
            allowed = true;
            break;
        }
    }

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

CommandHandler::CommandHandler(Bot* bot) : m_Bot(bot) {
    RegisterCommand("ship", CommandShip);
    RegisterCommand("flag", CommandFlag);
    RegisterCommand("freq", CommandFreq);
    RegisterCommand("taunt", CommandTaunt);
    RegisterCommand("target", CommandTarget);
    RegisterCommand("priority", CommandPriority);
    RegisterCommand("pause", CommandPause);
    RegisterCommand("commander", CommandCommander);
}

CommandHandler::~CommandHandler() {

}
