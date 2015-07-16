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

class WebSocketServer {
private:
    typedef websocketpp::server<websocketpp::config::asio> Server;
    typedef std::shared_ptr<std::thread> ThreadPtr;
    typedef Server::connection_ptr connection_ptr;
    typedef std::vector<connection_ptr> ClientList;

    Server m_Server;
    ThreadPtr m_Thread;
    ClientList m_Clients;
    std::mutex m_Mutex;

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

    void OnMessage(websocketpp::connection_hdl hdl, Server::message_ptr msg) {

    }

    void Send(const std::string& mesg) {
        std::lock_guard<std::mutex> guard(m_Mutex);
        
        for (connection_ptr ptr : m_Clients) {
            if (ptr->get_state() == websocketpp::session::state::open)
                m_Server.send(ptr.get()->get_handle(), mesg, websocketpp::frame::opcode::binary);
        }
    }

    void OnOpen(websocketpp::connection_hdl hdl) {
        std::lock_guard<std::mutex> guard(m_Mutex);

        connection_ptr ptr = m_Server.get_con_from_hdl(hdl);
        m_Clients.push_back(ptr);
    }

    void OnClose(websocketpp::connection_hdl hdl) {
        std::lock_guard<std::mutex> guard(m_Mutex);

        connection_ptr ptr = m_Server.get_con_from_hdl(hdl);
        m_Clients.erase(std::remove(m_Clients.begin(), m_Clients.end(), ptr), m_Clients.end());
    }
};

Json::Value SerializePlayer(api::PlayerPtr player) {
    Json::Value value;

    Vec2 pos = player->GetPosition();
    Vec2 velocity = player->GetVelocity();
    
    value["name"] = player->GetName();
    value["ship"] = (int)player->GetShip() + 1;
    value["freq"] = player->GetFreq();
    value["rot"] = player->GetRotation();
    value["pid"] = player->GetPid();

    value["pos"] = Json::Value(Json::objectValue);
    value["pos"]["x"] = pos.x;
    value["pos"]["y"] = pos.y;

    value["velocity"] = Json::Value(Json::objectValue);
    value["velocity"]["x"] = velocity.x;
    value["velocity"]["y"] = velocity.y;

    value["status"] = player->GetStatus();
    
    return value;
}

class WebPlugin : public Plugin {
private:
    api::Bot* m_Bot;
    const int UpdateFrequency = 100;
    int m_UpdateTimer;
    WebSocketServer m_Server;

public:
    WebPlugin(api::Bot* bot) : m_Bot(bot), m_UpdateTimer(0) { }
    ~WebPlugin() { }

    int OnCreate() {
        std::cout << "WebPlugin::OnCreate()" << std::endl;
        return 0;
    }

    int OnUpdate(unsigned long dt) {
        m_UpdateTimer += dt;

        if (m_UpdateTimer >= UpdateFrequency) {
            m_UpdateTimer = 0;

            api::PlayerList players = m_Bot->GetMemorySensor().GetPlayers();

            {
                Json::Value root;

                root["type"] = "players";
                root["players"] = Json::Value(Json::arrayValue);

                for (api::PlayerPtr player : players) {
                    Json::Value playerJSON = SerializePlayer(player);
                    root["players"].append(playerJSON);
                }

                std::string to_send = root.toStyledString();

                m_Server.Send(to_send);
            }


            {
                api::PlayerPtr target = m_Bot->GetEnemyTarget();
                api::StatePtr state = m_Bot->GetState();
                Json::Value root;

                if (target) {
                    root["type"] = "ai";
                    root["ai"]["target"] = target->GetName();

                    if (state->GetType() == api::StateType::ChaseState) {
                        ChaseState* chase = dynamic_cast<ChaseState*>(state.get());
                        Pathing::Plan plan = chase->GetPlan();

                        root["ai"]["plan"] = Json::Value(Json::arrayValue);

                        for (std::size_t i = 0; i < plan.size(); ++i) {
                            root["ai"]["plan"][i]["x"] = plan[i]->x;
                            root["ai"]["plan"][i]["y"] = plan[i]->y;
                        }
                    } else {
                        root["ai"]["plan"] = Json::Value(Json::arrayValue);
                        root["ai"]["plan"][0]["x"] = target->GetPosition().x / 16;
                        root["ai"]["plan"][0]["y"] = target->GetPosition().y / 16;
                    }

                    m_Server.Send(root.toStyledString());
                }
            }
        }

        return 0;
    }

    void OnChatMessage(ChatMessage* mesg) {
        std::vector<std::string> types = { "public", "private", "team", "channel", "other" };

        std::string player = mesg->GetPlayer();
        std::string message = mesg->GetMessage();

        Json::Value root;

        root["type"] = "chat";
        root["chat"] = Json::Value(Json::objectValue);
        root["chat"]["player"] = player;
        root["chat"]["message"] = message;
        root["chat"]["type"] = types.at((int)mesg->GetType());

        m_Server.Send(root.toStyledString());
    }

    int OnDestroy() {
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
