#include "api/Api.h"
#include "plugin/Plugin.h"
#include "CleverbotHTTP.h"

#include <algorithm>
#include <future>
#include <thread>
#include <queue>
#include <iostream>
#include <string>
#include <random>
#include <atomic>
#include <chrono>
#include <curl/curl.h>

/**
 * Simple plugin example. 
 * Both this plugin and the bot need to be compiled in release mode.
 * Statically link it by setting the configuration option in C/C++ -> Code Generation -> Runtime Library to Multi-threaded (/MT).
 */

namespace {

const ChatMessage::Type ChatType = ChatMessage::Type::Public;
const int Channel = 2;
const double BaseRespondChance = 0.1;

} // ns


namespace Random {

std::random_device dev;
std::mt19937 gen(dev());

double GetReal() {
    std::uniform_real_distribution<double> dist(0, 1);
    return dist(gen);
}

} // ns


std::string strtolower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), tolower);
    return str;
}

class CleverbotQueue {
private:
    const int m_TimeoutMS = 8 * 1000;

    std::shared_ptr<CleverbotHTTP> m_Cleverbot;

    std::shared_ptr<std::thread> m_Thread;
    std::queue<std::string> m_Queue;
    std::mutex m_Mutex;
    api::Bot* m_Bot;
    std::atomic<bool> m_Running;

    ChatMessage::Type m_Type;
    int m_Channel;


    void StripEntities(std::string& str) {
        using namespace std;

        while (true) {
            size_t begin = str.find("&");
            size_t end = str.find(";");

            if (begin == string::npos || end == string::npos || begin >= end) return;
            str.erase(begin, end - begin + 1);
        }
    }

    bool Contains(std::string str, const std::string& find) {
        std::string find_lower = strtolower(find);
        str = strtolower(str);

        return str.find(find_lower) != std::string::npos;
    }

    std::string EraseName(std::string thought) {
        std::string bot_name = strtolower(m_Bot->GetName());
        thought = strtolower(thought);

        // Don't erase name if it's the only thing in the line
        if (thought.compare(bot_name) == 0) return thought;

        std::size_t pos = thought.find(bot_name);

        if (pos != std::string::npos)
            thought.erase(pos, bot_name.length());

        if (thought.length() > pos && thought[pos] == ' ')
            thought.erase(pos, 1);

        return thought;
    }

    void HandleResponse(std::string response) {
        static const std::vector<std::string> filter = { "clever", "ios app" };

        std::cout << "Response: " << response << std::endl;

        if (response.length() > 0) {
            if (response.at(0) == '*')
                response.insert(response.begin(), '.');

            for (const std::string& str : filter)
                if (Contains(response, str)) return;

            StripEntities(response);

            std::string to_send;

            switch (m_Type) {
                case ChatMessage::Type::Channel:
                    to_send = ";";
                    if (Channel > 1)
                        to_send += std::to_string(m_Channel) + ";";
                break;
                case ChatMessage::Type::Team:
                    to_send = "'";
                break;
            }

            to_send += response;
            m_Bot->SendMessage(to_send);
        }
    }

    void Run() {
        m_Running = true;
        while (m_Running) {
            // Load it in this thread so main thread doesn't block
            if (!m_Cleverbot)
                m_Cleverbot = std::make_shared<CleverbotHTTP>();

            m_Mutex.lock();
            if (m_Queue.empty()) {
                m_Mutex.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            std::string thought = m_Queue.front();
            m_Queue.pop();
            m_Mutex.unlock();

            thought = EraseName(thought);

            std::cout << "Thinking: " << thought << std::endl;

            std::future<std::string> future = std::async(std::launch::async, std::bind(&CleverbotHTTP::Think, m_Cleverbot.get(), thought));
            if (future.wait_for(std::chrono::milliseconds(m_TimeoutMS)) != std::future_status::ready) {
                std::cout << "Timeout." << std::endl;
                continue;
            }

            HandleResponse(future.get());
        }
    }

public:
    CleverbotQueue(api::Bot* bot, ChatMessage::Type type, int channel) : m_Bot(bot), m_Queue(), m_Mutex(), m_Type(type), m_Channel(channel) {
        m_Thread = std::make_shared<std::thread>(&CleverbotQueue::Run, this);
    }

    ~CleverbotQueue() {
        m_Running = false;
        m_Thread->join();
    }

    void Enqueue(const std::string& str) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (m_Queue.size() < 2)
            m_Queue.push(str);
    }
};

class CleverbotPlugin : public Plugin {
private:
    typedef std::chrono::system_clock::rep Timestamp;

    api::Bot* m_Bot;
    std::shared_ptr<CleverbotQueue> m_CBQ;
    bool m_LastReference; // True if the last message not sent by bot had the bot name in it

    struct Message {
        std::string player;
        std::string message;

        Message(ChatMessage* mesg) : player(mesg->GetPlayer()), message(mesg->GetMessage()) { }
    };

    struct StoredMessage {
        Message message;
        Timestamp timestamp;

        StoredMessage(ChatMessage* mesg, Timestamp timestamp) : message(mesg), timestamp(timestamp) { }
    };

    typedef std::deque<StoredMessage> ChatQueue;
    ChatQueue m_ChatQueue;

public:
    CleverbotPlugin(api::Bot* bot) : m_Bot(bot) { }

    int OnCreate() {
        m_CBQ = std::make_shared<CleverbotQueue>(m_Bot, ChatType, Channel);

        std::string version = m_Bot->GetVersion().GetString();
        std::cout << "Cleverbot loaded for monkeybot version " << version << std::endl;

        return 0;
    }

    Timestamp GetTime() const {
        using namespace std::chrono;

        auto duration = system_clock::now().time_since_epoch();
        return duration_cast<milliseconds>(duration).count();
    }

    int OnUpdate(unsigned long dt) {
        return 0;
    }

    int OnDestroy() {
        return 0;
    }

    void PurgeMessages() {
        const Timestamp AliveTime = 1000 * 60 * 3;
        const Timestamp timestamp = GetTime();

        while (!m_ChatQueue.empty()) {
            StoredMessage message = m_ChatQueue.front();
            if (timestamp - message.timestamp < AliveTime)
                break;
            m_ChatQueue.pop_front();
        }
    }

    std::size_t GetReferenceCount() const {
        ChatQueue::const_iterator iter = m_ChatQueue.begin();
        std::string name = strtolower(m_Bot->GetName());
        std::size_t references = 0;

        while (iter != m_ChatQueue.end()) {
            std::string message = strtolower(iter->message.message);

            if (message.find(name) != std::string::npos)
                ++references;

            ++iter;
        }

        return references;
    }

    std::size_t GetChatterCount() const {
        std::size_t chatters = m_ChatQueue.size();
        return chatters;
    }

    double GetRespondChance() const {
        std::size_t chatters = GetChatterCount();

        if (chatters <= 1 || m_LastReference) return 1.0;

        std::size_t references = GetReferenceCount();

        return BaseRespondChance + (1.0 / (chatters * 1.75)) + ((references / (double)chatters) * .50);
    }

    void OnChatMessage(ChatMessage* mesg) {
        using namespace std;

        if (mesg->GetType() != ChatType) return;
        if (ChatType == ChatMessage::Type::Channel && mesg->GetChannel() != Channel)  return;
        if (mesg->GetPlayer().compare(m_Bot->GetName()) == 0) return;

        PurgeMessages();

        string message = strtolower(mesg->GetMessage());
        string name = strtolower(m_Bot->GetName());
        double respond_chance = GetRespondChance();
        bool contains_name = message.find(name) != string::npos;

        if (contains_name)
            m_ChatQueue.clear();

        StoredMessage stored(mesg, GetTime());

        bool push = m_ChatQueue.empty() || (!m_ChatQueue.empty() && m_ChatQueue.back().message.player.compare(mesg->GetPlayer()) != 0);
        if (push)
            m_ChatQueue.push_back(stored);
        
        std::cout << "Chatters: " << GetChatterCount() << " References: " << GetReferenceCount() << " Respond Chance: " << respond_chance << std::endl;

        if (contains_name || Random::GetReal() <= respond_chance)
            m_CBQ->Enqueue(mesg->GetMessage());

        m_LastReference = contains_name;
    }
};


extern "C" {
    PLUGIN_FUNC Plugin* CreatePlugin(api::Bot* bot) {
        curl_global_init(CURL_GLOBAL_ALL);
        return new CleverbotPlugin(bot);
    }

    PLUGIN_FUNC void DestroyPlugin(Plugin* plugin) {
        delete plugin;
        curl_global_cleanup();
    }

    PLUGIN_FUNC const char* GetPluginName() {
        return "CleverbotPlugin";
    }
}
