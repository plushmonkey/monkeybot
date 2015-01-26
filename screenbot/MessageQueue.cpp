#include "MessageQueue.h"

MessageQueue& MessageQueue::GetInstance() {
    static MessageQueue mq;
    return mq;
}

void MessageQueue::RegisterDispatcher(MessageDispatcher* dispatcher) {
    m_Dispatchers.push_back(dispatcher);
}

void MessageQueue::Dispatch() {
    for (auto iter = m_Dispatchers.begin(); iter != m_Dispatchers.end(); ++iter) {
        MessageDispatcher* dispatcher = *iter;

        if (dispatcher)
            dispatcher->Dispatch();
    }
}

void MessageQueue::Discard() {
    for (auto iter = m_Dispatchers.begin(); iter != m_Dispatchers.end(); ++iter) {
        MessageDispatcher* dispatcher = *iter;

        if (dispatcher)
            dispatcher->Discard();
    }
}

void MessageQueue::Reset() {
    for (auto iter = m_Dispatchers.begin(); iter != m_Dispatchers.end(); ++iter) {
        MessageDispatcher* dispatcher = *iter;

        if (dispatcher)
            dispatcher->Reset();
    }
}
