#ifndef LOG_READER_H_
#define LOG_READER_H_

#include "Common.h"
#include "FileStream.h"

#include <string>

class ChatMessage;

class LogReader {
private:
    FileStream m_FileStream;
    DWORD m_UpdateTimer;
    unsigned int m_UpdateFrequency;
    
    ChatMessage* GetChatMessage(const std::string& line);

public:
    LogReader(const std::string& filename, const unsigned int frequency);

    void Clear();
    void Update(unsigned long dt);
};

#endif
