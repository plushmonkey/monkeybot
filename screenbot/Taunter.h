#ifndef TAUNTER_H_
#define TAUNTER_H_

#include "Messages/Messages.h"

class Bot;

class Taunter : public MessageHandler<KillMessage> {
private:
    Bot* m_Bot;
    bool m_Enabled;
    unsigned int m_LastTaunt;
    virtual void HandleMessage(KillMessage* mesg);

public:
    Taunter(Bot* bot);

    void SetEnabled(bool b) { m_Enabled = b; }
};

#endif
