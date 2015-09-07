#ifndef MOVEMENT_MANAGER_H_
#define MOVEMENT_MANAGER_H_

#include "api/Api.h"

class MovementManager : public api::MovementManager {
private:
    ClientPtr m_Client;
    api::SteeringBehavior* m_Steering;
    bool m_Reverse;
    bool m_Enabled;

public:
    MovementManager(ClientPtr client, api::SteeringBehavior* steering);

    void UseReverse(bool use);
    bool IsUsingReverse() const;
    void SetEnabled(bool enable);
    bool IsEnabled() const;

    void Update(api::Bot* bot, unsigned long dt);
};

#endif
