#ifndef SHIP_ENFORCER_H_
#define SHIP_ENFORCER_H_

#include "api/Api.h"

class ShipEnforcer {
private:
    api::Bot::UpdateID m_UpdateID;
    api::Bot* m_Bot;

public:
    bool OnUpdate(api::Bot* bot, unsigned long dt);

    ShipEnforcer(api::Bot* bot);
    ~ShipEnforcer();
};

#endif
