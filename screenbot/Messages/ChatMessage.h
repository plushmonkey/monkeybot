#ifndef CHAT_MESSAGE_H_
#define CHAT_MESSAGE_H_

#include <string>
#include "../MessageQueue.h"

class ChatMessage : public Message {
private:
    std::string m_Message;

public:
    ChatMessage(const std::string& line) : m_Message(line) { }
    std::string GetLine() const { return m_Message; }
};

#endif
