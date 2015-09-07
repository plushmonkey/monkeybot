#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_

#include "api/Api.h"
#include <stack>

class StateMachine : public api::StateMachine {
private:
    std::stack<api::StatePtr> m_Stack;

public:
    void Update(DWORD dt);
    void Push(api::StatePtr state);
    api::StatePtr Pop();
    api::StatePtr GetState() const;
    bool IsEmpty() const;
    void Clear();
};

#endif
