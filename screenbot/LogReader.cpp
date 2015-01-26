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
        MQueue.PushMessage(new ChatMessage(line));

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
