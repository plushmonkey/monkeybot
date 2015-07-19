#include <boost/asio.hpp>

#include "api/Api.h"
#include "plugin/Plugin.h"
#include "Vector.h"
#include "State.h"

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <json/json.h>
#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>
#include <mutex>
#include <map>

namespace websocket {

typedef websocketpp::server<websocketpp::config::asio> Server;

} // ns

class WebSocketListener {
private:
public:
    virtual void OnOpen(websocketpp::connection_hdl hdl) { }
    virtual void OnClose(websocketpp::connection_hdl hdl) { }
    virtual void OnMessage(websocketpp::connection_hdl hdl, websocket::Server::message_ptr msg) { }
};

class WebSocketServer {
private:
    typedef std::shared_ptr<std::thread> ThreadPtr;
    typedef websocket::Server::connection_ptr connection_ptr;
    typedef std::vector<connection_ptr> ClientList;

    websocket::Server m_Server;
    ThreadPtr m_Thread;
    ClientList m_Clients;
    std::mutex m_Mutex;
    std::vector<WebSocketListener*> m_Listeners;

    WebSocketServer(const WebSocketServer&);
    WebSocketServer& operator=(const WebSocketServer&);

public:
    WebSocketServer() {
        m_Thread = ThreadPtr(new std::thread(std::bind(&WebSocketServer::Run, this)));
    }

    ~WebSocketServer() {
        std::lock_guard<std::mutex> guard(m_Mutex);

        m_Clients.clear();
        m_Server.stop();
        m_Thread->join();
    }

    void RegisterListener(WebSocketListener* listener) {
        m_Listeners.push_back(listener);
    }

    void UnregisterListener(WebSocketListener* listener) {
        m_Listeners.erase(std::remove(m_Listeners.begin(), m_Listeners.end(), listener), m_Listeners.end());
    }

    void Run() {
        using websocketpp::lib::placeholders::_1;
        using websocketpp::lib::placeholders::_2;
        using websocketpp::lib::bind;

        m_Server.clear_access_channels(websocketpp::log::alevel::all);
        m_Server.set_access_channels(websocketpp::log::alevel::connect);
        m_Server.set_access_channels(websocketpp::log::alevel::disconnect);

        m_Server.init_asio();
        m_Server.set_message_handler(bind(&WebSocketServer::OnMessage, this, ::_1, ::_2));
        m_Server.set_open_handler(bind(&WebSocketServer::OnOpen, this, ::_1));
        m_Server.set_close_handler(bind(&WebSocketServer::OnClose, this, ::_1));
        m_Server.listen(boost::asio::ip::tcp::v4(), 9002);
        m_Server.start_accept();
        m_Server.run();
    }

    void OnMessage(websocketpp::connection_hdl hdl, websocket::Server::message_ptr msg) {
        for (auto listener : m_Listeners)
            listener->OnMessage(hdl, msg);
    }

    void Send(const std::string& mesg) {
        std::lock_guard<std::mutex> guard(m_Mutex);
        
        for (connection_ptr ptr : m_Clients) {
            if (ptr->get_state() == websocketpp::session::state::open)
                m_Server.send(ptr.get()->get_handle(), mesg, websocketpp::frame::opcode::binary);
        }
    }

    void Send(const std::string& mesg, websocketpp::connection_hdl hdl) {
        m_Server.send(hdl, mesg, websocketpp::frame::opcode::binary);
    }

    void OnOpen(websocketpp::connection_hdl hdl) {
        std::lock_guard<std::mutex> guard(m_Mutex);

        connection_ptr ptr = m_Server.get_con_from_hdl(hdl);
        m_Clients.push_back(ptr);

        for (auto listener : m_Listeners)
            listener->OnOpen(hdl);
    }

    void OnClose(websocketpp::connection_hdl hdl) {
        std::lock_guard<std::mutex> guard(m_Mutex);

        connection_ptr ptr = m_Server.get_con_from_hdl(hdl);
        m_Clients.erase(std::remove(m_Clients.begin(), m_Clients.end(), ptr), m_Clients.end());

        for (auto listener : m_Listeners)
            listener->OnClose(hdl);
    }
};

enum class PacketType {
    PlayerData,
    Disconnect
};

#pragma pack(push, 1)
struct PlayerData {
    unsigned short type;

    char name[20];
    unsigned short pid;
    unsigned char ship;
    unsigned int freq;
    unsigned char rotation;
    double x;
    double y;
    double velocity_x;
    double velocity_y;
    unsigned char status;

    bool operator==(const PlayerData& other) const {
        return memcmp(this, &other, sizeof(PlayerData)) == 0;
    }

    bool operator!=(const PlayerData& other) const {
        return !(*this == other);
    }
};

struct Disconnect {
    unsigned short type;
    unsigned short pid;
};
#pragma pack(pop)

PlayerData SerializePlayer(api::PlayerPtr player) {
    PlayerData data;

    Vec2 pos = player->GetPosition();
    Vec2 velocity = player->GetVelocity();

    data.type = (int)PacketType::PlayerData;
    memset(data.name, 0, sizeof(data.name));
    strcpy(data.name, player->GetName().c_str());
    data.pid = player->GetPid();
    data.ship = (int)player->GetShip() + 1;
    data.freq = player->GetFreq();
    data.rotation = (unsigned char)player->GetRotation();
    data.x = pos.x;
    data.y = pos.y;
    data.velocity_x = velocity.x;
    data.velocity_y = velocity.y;
    data.status = player->GetStatus();

    return data;
}

Disconnect SerializeDisconnect(api::PlayerPtr player) {
    Disconnect disconnect;

    disconnect.type = (short)PacketType::Disconnect;
    disconnect.pid = player->GetPid();

    return disconnect;
}

Json::Value SerializeChat(ChatMessage* mesg) {
    static const std::vector<std::string> types = { "public", "private", "team", "channel", "other" };

    std::string player = mesg->GetPlayer();
    std::string message = mesg->GetMessage();

    Json::Value root;

    root["type"] = "chat";
    root["chat"] = Json::Value(Json::objectValue);
    root["chat"]["player"] = player;
    root["chat"]["message"] = message;
    root["chat"]["type"] = types.at((int)mesg->GetType());

    return root;
}

Json::Value SerializeAI(api::Bot* bot) {
    api::PlayerPtr target = bot->GetEnemyTarget();
    api::StatePtr state = bot->GetState();
    Json::Value root;

    if (target) {
        root["type"] = "ai";
        root["ai"]["target"] = target->GetName();

        api::StateType type = state->GetType();

        if (type == api::StateType::ChaseState) {
            ChaseState* chase = dynamic_cast<ChaseState*>(state.get());
            Pathing::Plan plan = chase->GetPlan();

            root["ai"]["plan"] = Json::Value(Json::arrayValue);

            for (std::size_t i = 0; i < plan.size(); ++i) {
                root["ai"]["plan"][i]["x"] = plan[i]->x;
                root["ai"]["plan"][i]["y"] = plan[i]->y;
            }
        } else if (type == api::StateType::AggressiveState) {
            root["ai"]["plan"] = Json::Value(Json::arrayValue);
            root["ai"]["plan"][0]["x"] = target->GetPosition().x / 16;
            root["ai"]["plan"][0]["y"] = target->GetPosition().y / 16;
        }
    }

    return root;
}

class WebPlugin : public Plugin, public WebSocketListener {
private:
    api::Bot* m_Bot;
    const int UpdateFrequency = 100;
    int m_UpdateTimer;
    WebSocketServer m_Server;
    std::map<std::string, PlayerData> m_LastData;

    void UpdatePlayers() {
        api::PlayerList players = m_Bot->GetMemorySensor().GetPlayers();
        std::string data;

        for (api::PlayerPtr player : players) {
            PlayerData lastData = m_LastData[player->GetName()];
            PlayerData pdata = SerializePlayer(player);

            if (lastData != pdata)
                data.append((char*)&pdata, sizeof(PlayerData));
            m_LastData[player->GetName()] = pdata;
        }

        if (data.length() > 0)
            m_Server.Send(data);
    }

public:
    WebPlugin(api::Bot* bot) : m_Bot(bot), m_UpdateTimer(0) { }
    ~WebPlugin() { }

    void OnOpen(websocketpp::connection_hdl hdl) {
        // Send full player data
        api::PlayerList players = m_Bot->GetMemorySensor().GetPlayers();
        std::string data;

        for (api::PlayerPtr player : players) {
            PlayerData pdata = SerializePlayer(player);

            data.append((char*)&pdata, sizeof(PlayerData));
        }
    }

    int OnCreate() {
        std::cout << "WebPlugin::OnCreate()" << std::endl;

        m_Server.RegisterListener(this);

        return 0;
    }

    int OnUpdate(unsigned long dt) {
        m_UpdateTimer += dt;

        if (m_UpdateTimer >= UpdateFrequency) {
            m_UpdateTimer = 0;

            UpdatePlayers();

            std::string ai = SerializeAI(m_Bot).toStyledString();
            if (ai.length() > 0)
                m_Server.Send(ai);
        }

        return 0;
    }

    void OnChatMessage(ChatMessage* mesg) {
        Json::Value chat = SerializeChat(mesg);

        m_Server.Send(chat.toStyledString());
    }

    void OnLeave(LeaveMessage* mesg) {
        api::PlayerPtr player = mesg->GetPlayer();
        Disconnect disconnect = SerializeDisconnect(player);
        std::string data;

        data.append((char*)&disconnect, sizeof(Disconnect));

        m_Server.Send(data);
    }

    int OnDestroy() {
        m_Server.UnregisterListener(this);
        std::cout << "WebPlugin::OnDestroy()" << std::endl;
        return 0;
    }
};

extern "C" {
    PLUGIN_FUNC Plugin* CreatePlugin(api::Bot* bot) {
        return new WebPlugin(bot);
    }

    PLUGIN_FUNC void DestroyPlugin(Plugin* plugin) {
        delete plugin;
    }

    PLUGIN_FUNC const char* GetPluginName() {
        return "Web";
    }
}
