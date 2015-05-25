#ifndef SELECTOR_H_
#define SELECTOR_H_

#include "api/Api.h"

// Targets the closest enemy
class ClosestEnemySelector : public api::Selector {
public:
    std::shared_ptr<Player> Select(api::Bot* bot);
};

// Targets the enemy set with !target command
class TargetEnemySelector : public api::Selector {
private:
    ClosestEnemySelector m_ClosestSelector;
    std::string m_Target;

public:
    TargetEnemySelector(const std::string& name);

    std::shared_ptr<Player> Select(api::Bot* bot);
};

// 
class AttachSelector : public api::Selector {
public:
    std::shared_ptr<Player> Select(api::Bot* bot);
};

#endif
