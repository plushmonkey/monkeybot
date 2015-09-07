#include "StateMachine.h"

void StateMachine::Update(DWORD dt) {
    if (m_Stack.empty()) return;

    m_Stack.top()->Update(dt);
}

void StateMachine::Push(api::StatePtr state) {
    m_Stack.push(state);
}

api::StatePtr StateMachine::Pop() {
    api::StatePtr state = m_Stack.top();
    m_Stack.pop();

    return state;
}

api::StatePtr StateMachine::GetState() const {
    return m_Stack.top();
}

bool StateMachine::IsEmpty() const {
    return m_Stack.empty();
}

void StateMachine::Clear() {
    while (!m_Stack.empty())
        m_Stack.pop();
}
