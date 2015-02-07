#ifndef CHAT_MESSAGE_H_
#define CHAT_MESSAGE_H_

#include "../MessageQueue.h"

#include <string>

#ifdef GetMessage
#undef GetMessage
#endif

class ChatMessage : public Message {
public:
    enum class Type { Public, Private, Team, Channel, Other };

private:
    std::string m_Message;
    std::string m_Player;
    Type m_Type;
    int m_Channel;

public:
    ChatMessage() : m_Type(Type::Other), m_Channel(0) { }

    const std::string& GetMessage() const { return m_Message; }
    const std::string& GetPlayer() const { return m_Player; }
    Type GetType() const { return m_Type; }
    int GetChannel() const { return m_Channel; }

    void SetMessage(const std::string& mesg) { m_Message = mesg; }
    void SetPlayer(const std::string& player) { m_Player = player; }
    void SetType(Type t) { m_Type = t; }
    void SetChannel(int channel) { m_Channel = channel; }
};

#endif
