#include "api/Api.h"
#include "plugin/Plugin.h"

#include <algorithm>
#include <future>
#include <thread>
#include <queue>
#include <iostream>
#include <string>
#include <random>
#include <atomic>

/**
 * Simple plugin example. 
 * Both this plugin and the bot need to be compiled in release mode.
 * Statically link it by setting the configuration option in C/C++ -> Code Generation -> Runtime Library to Multi-threaded (/MT).
 * 
 * Cleverbot needs to be installed for python using pip. 'pip install cleverbot'
 * Put the cbot.py in the same directory as the screenbot.exe.
 */

namespace {

const ChatMessage::Type ChatType = ChatMessage::Type::Public;
const int Channel = 2;
const double RespondChance = 0.2;

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


class Cleverbot {
public:
    static std::string Think(std::string thought) {
        thought.erase(std::remove(thought.begin(), thought.end(), '"'));

        std::string program("python cbot.py");
        std::string run = program + " \"" + thought + "\"";

        FILE *pipe = _popen(run.c_str(), "r");
        if (!pipe) return "";

        std::string result;
        char buffer[200];
        while (!feof(pipe)) {
            if (fgets(buffer, 200, pipe) != nullptr)
                result += buffer;
        }
        _pclose(pipe);

        result.erase(std::remove(result.begin(), result.end(), '\r'));
        result.erase(std::remove(result.begin(), result.end(), '\n'));
        return result;
    }
};

class CleverbotQueue {
private:
    const int m_TimeoutMS = 10 * 1000;

    std::shared_ptr<std::thread> m_Thread;
    std::queue<std::string> m_Queue;
    std::mutex m_Mutex;
    api::Bot* m_Bot;
    std::atomic<bool> m_Running;

    ChatMessage::Type m_Type;
    int m_Channel;


    void StripEntities(std::string& str) {
        using namespace std;
        size_t begin = str.find("&");
        size_t end = str.find(";");

        if (begin == string::npos || end == string::npos || begin >= end) return;
        str.erase(begin, end - begin + 1);
    }

    bool Contains(std::string str, const std::string& find) {
        std::string find_lower = strtolower(find);
        str = strtolower(str);

        return str.find(find_lower) != std::string::npos;
    }

    void Run() {
        m_Running = true;
        while (m_Running) {
            m_Mutex.lock();
            if (m_Queue.empty()) {
                m_Mutex.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            std::string thought = m_Queue.front();
            m_Queue.pop();
            m_Mutex.unlock();

            std::future<std::string> future = std::async(std::launch::async, std::bind(Cleverbot::Think, thought));
            if (future.wait_for(std::chrono::milliseconds(m_TimeoutMS)) != std::future_status::ready) {
                std::cout << "Timeout." << std::endl;
                continue;
            }
            std::string response = future.get();
            if (response.length() > 0) {
                if (response.at(0) == '*')
                    response.insert(response.begin(), '.');

                if (Contains(response, "clever")) continue;
                if (Contains(response, "ios app")) continue;

                StripEntities(response);

                std::string to_send;

                switch (m_Type) {
                case ChatMessage::Type::Channel:
                    to_send = ";" + std::to_string(m_Channel) + ";";
                    break;
                case ChatMessage::Type::Team:
                    to_send = "'";
                    break;
                }

                to_send += response;
                m_Bot->SendMessage(to_send);
            }
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
    api::Bot* m_Bot;
    std::shared_ptr<CleverbotQueue> m_CBQ;

public:
    CleverbotPlugin(api::Bot* bot) : m_Bot(bot) { }

    int OnCreate() {
        std::string version = m_Bot->GetVersion().GetString();

        std::cout << "Cleverbot loaded for monkeybot version " << version << std::endl;
        return 0;
    }

    int OnUpdate(unsigned long dt) {
        return 0;
    }

    int OnDestroy() {
        return 0;
    }

    void OnChatMessage(ChatMessage* mesg) {
        if (mesg->GetType() != ChatType || mesg->GetPlayer().compare(m_Bot->GetName()) == 0)
            return;
        if (!m_CBQ.get())
            m_CBQ = std::make_shared<CleverbotQueue>(m_Bot, ChatType, Channel);

        if (ChatType == ChatMessage::Type::Channel && mesg->GetChannel() != Channel) return;

        std::string message = strtolower(mesg->GetMessage());
        std::string name = strtolower(m_Bot->GetName());

        bool contains_name = message.find(name) != std::string::npos;
        if (contains_name || Random::GetReal() <= RespondChance) {
            if (m_CBQ.get())
                m_CBQ->Enqueue(mesg->GetMessage());
        }
    }
};


extern "C" {
    PLUGIN_FUNC Plugin* CreatePlugin(api::Bot* bot) {
        return new CleverbotPlugin(bot);
    }

    PLUGIN_FUNC void DestroyPlugin(Plugin* plugin) {
        delete plugin;
    }

    PLUGIN_FUNC const char* GetPluginName() {
        return "CleverbotPlugin";
    }
}
