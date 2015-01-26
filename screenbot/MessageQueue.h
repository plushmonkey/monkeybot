#ifndef MESSAGE_QUEUE_H_
#define MESSAGE_QUEUE_H_

#include <vector>
#include <list>
#include <memory>

template <typename T>
class MessageDispatcherImpl;

class Message {
public:
    virtual ~Message() { }
};

template <typename T>
class MessageHandler {
protected:
    MessageHandler() {
        MessageDispatcherImpl<T>::GetInstance().RegisterHandler(this);
    }

public:
    virtual ~MessageHandler() {
        MessageDispatcherImpl<T>::GetInstance().UnregisterHandler(this);
    }
    virtual void HandleMessage(T* message) = 0;
};

class MessageDispatcher {
public:
    virtual void Dispatch() = 0;
    virtual void Discard() = 0;
    virtual void Reset() = 0;
};

template <typename T>
class MessageDispatcherImpl : public MessageDispatcher {
private:
    typedef T MessageImpl;
    typedef std::vector<std::shared_ptr<MessageImpl>> MessageVector;
    typedef MessageHandler<T> MessageHandlerImpl;
    typedef std::vector<MessageHandlerImpl*> MessageHandlerVector;

    MessageVector m_Messages;
    MessageHandlerVector m_Handlers;

    MessageDispatcherImpl() {
        MessageQueue::GetInstance().RegisterDispatcher(this);
    }

    virtual ~MessageDispatcherImpl() { }

public:
    static MessageDispatcherImpl& GetInstance() {
        static MessageDispatcherImpl instance;
        return instance;
    }

    virtual void Dispatch() {
        if (m_Handlers.size() > 0 && m_Messages.size() > 0) {
            for (auto mesg = m_Messages.begin(); mesg != m_Messages.end(); ++mesg) {
                for (auto handler = m_Handlers.begin(); handler != m_Handlers.end(); ++handler) {
                    if (*handler)
                        (*handler)->HandleMessage(mesg->get());
                }
            }
        }
        Discard();
    }

    virtual void Discard() {
        m_Messages.clear();
    }

    virtual void Reset() {
        m_Messages.clear();
        m_Handlers.clear();
    }

    void PushMessage(T* mesg) {
        std::shared_ptr<T> ptr(mesg);
        m_Messages.push_back(ptr);
    }

    void RegisterHandler(MessageHandlerImpl* handler) {
        for (size_t i = 0; i < m_Handlers.size(); ++i) {
            if (m_Handlers[i] == NULL && handler) {
                m_Handlers[i] = handler;
                return;
            }
        }
        m_Handlers.push_back(handler);
    }

    void UnregisterHandler(MessageHandlerImpl* handler) {
        for (size_t i = 0; i < m_Handlers.size(); ++i) {
            if (m_Handlers[i] == handler) {
                m_Handlers[i] = NULL;
                return;
            }
        }
    }
};

class MessageQueue {
private:
    std::list<MessageDispatcher*> m_Dispatchers;

    MessageQueue() { }
    ~MessageQueue() { }

public:
    static MessageQueue& GetInstance();
    void RegisterDispatcher(MessageDispatcher* dispatcher);
    void Dispatch();
    void Discard();
    void Reset();

    template <class T>
    void PushMessage(T* mesg) {
        MessageDispatcherImpl<T>::GetInstance().PushMessage(mesg);
    }
};

#define MQueue MessageQueue::GetInstance()

#endif
