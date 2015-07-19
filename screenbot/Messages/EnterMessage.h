#ifndef ENTER_MESSAGE_H_
#define ENTER_MESSAGE_H_

#include <string>
#include "../MessageQueue.h"
#include "../api/Player.h"

class EnterMessage : public Message {
private:
    api::PlayerPtr m_Player;

public:
    EnterMessage(api::PlayerPtr player)
        : m_Player(player)
    {

    }

    api::PlayerPtr GetPlayer() const { return m_Player; }
};


class LeaveMessage : public Message {
private:
    api::PlayerPtr m_Player;

public:
    LeaveMessage(api::PlayerPtr player)
        : m_Player(player)
    {

    }

    api::PlayerPtr GetPlayer() const { return m_Player; }
};

#endif
