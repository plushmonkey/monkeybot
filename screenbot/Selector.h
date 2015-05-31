#ifndef SELECTOR_H_
#define SELECTOR_H_

#include "api/Api.h"
#include "Player.h"
#include "Util.h"
#include "MemorySensor.h"
#include <limits>
//#include "Config.h"

// Targets the closest enemy
class ClosestEnemySelector : public api::Selector {
public:
    std::shared_ptr<api::Player> Select(api::Bot* bot);
};

// Targets the enemy set with !target command
class TargetEnemySelector : public api::Selector {
private:
    ClosestEnemySelector m_ClosestSelector;
    std::string m_Target;

public:
    TargetEnemySelector(const std::string& name);

    std::shared_ptr<api::Player> Select(api::Bot* bot);
};

// 
class AttachSelector : public api::Selector {
public:
    std::shared_ptr<api::Player> Select(api::Bot* bot);
};

class EnemySelectorFactory : public api::EnemySelectorFactory {
public:
    api::SelectorPtr CreateClosest();
    api::SelectorPtr CreateTarget(const std::string& target);
};

#endif
