#include "LogReader.h"

#include "Tokenizer.h"
#include "Messages/Messages.h"

#include <regex>


const std::regex KillRE(R"::(^\s+(.+)\(([0-9]+)\) killed by: (.+)$)::");

LogReader::LogReader(const std::string& filename, const unsigned int frequency) 
    : m_FileStream(filename),
      m_UpdateFrequency(frequency),
      m_UpdateTimer(0)
{
    std::ifstream in(filename, std::ios::in);

    if (!in.is_open())
        std::cout << "Could not open " << filename << " for reading." << std::endl;
    else
        in.close();
}

ChatMessage* LogReader::GetChatMessage(const std::string& line) {
    const std::regex chat_regex(R"::(^(P|T|C|\s)\s+([0-9]+:|\s)(.+)> (.+)$)::");
    std::sregex_iterator begin(line.begin(), line.end(), chat_regex);
    std::sregex_iterator end;

    ChatMessage* mesg = new ChatMessage;

    if (begin == end) {
        /* Probably arena message or kill message, just send the raw message */
        mesg->SetType(ChatMessage::Type::Other);
        mesg->SetMessage(line);
        return mesg;
    }

    std::smatch match = *begin;

    std::string typestr = match[1];
    std::string player = match[3];
    std::string message = match[4];
    ChatMessage::Type type;

    if (typestr.at(0) == 'P') 
        type = ChatMessage::Type::Private;
    else if (typestr.at(0) == 'T')
        type = ChatMessage::Type::Team;
    else if (typestr.at(0) == 'C')
        type = ChatMessage::Type::Channel;
    else
        type = ChatMessage::Type::Public;

    if (type == ChatMessage::Type::Channel)
        mesg->SetChannel(atoi(std::string(match[2]).c_str()));

    mesg->SetPlayer(player);
    mesg->SetMessage(message);
    mesg->SetType(type);

    return mesg;
}

void LogReader::Update(unsigned long dt) {
    m_UpdateTimer += dt;

    if (m_UpdateTimer < m_UpdateFrequency) return;
    m_UpdateTimer = 0;

    std::string data;

    m_FileStream >> data;
    if (data.length() == 0) return;

    Util::Tokenizer tokens(data);
    tokens('\n');

    for (auto line : tokens) {
        if (line.length() == 0) continue;

        ChatMessage* mesg = GetChatMessage(line);
        MQueue.PushMessage(mesg);

        std::sregex_iterator begin(line.begin(), line.end(), KillRE);
        std::sregex_iterator end;

        if (begin != end) {
            std::smatch match = *begin;
            std::string killer = match[3];
            int bounty = atoi(std::string(match[2]).c_str());
            std::string killed = match[1];

            MQueue.PushMessage(new KillMessage(killer, killed, bounty));
        }
    }
}

void LogReader::Clear() {
    std::string data;
    m_FileStream >> data;
}
