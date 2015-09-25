#ifndef CONFIG_H_
#define CONFIG_H_

#include "Common.h"

#include "Vector.h"

#include <string>
#include <vector>
#include <map>
#include <iosfwd>
#include <json/json.h>
#include <type_traits>
#include <utility>
#include <typeinfo>
#include <string>
#include <cassert>
#include "api/Config.h"

class Config  {
private:
    Json::Value m_Root;

    void LoadFromNode(const Json::Value& node);

public:
    int XPercent;
    int RunPercent;
    int TargetDistance;
    int RunDistance;
    int StopBombing;
    int BombDelay;
    bool FireBombs;
    bool FireGuns;
    std::string Level;
    int BulletDelay;
    bool ScaleDelay;
    bool CenterOnly;
    bool Patrol;
    bool Attach;
    int MinGunRange;
    int SpawnX;
    int SpawnY;
    std::vector<Vec2> Waypoints;
    bool Baseduel;
    int CenterRadius;
    int IgnoreDelayDistance;
    int RepelPercent;
    bool UseBurst;
    unsigned int DecoyDelay;
    bool Taunt;
    unsigned int TauntCooldown;
    std::vector<std::string> Taunts;
    std::vector<std::string> TauntWhitelist;
    bool Hyperspace;
    bool Commander;
    bool Survivor;
    std::string Zone;
    bool MultiFire;
    bool Revenge;
    std::map<std::string, std::vector<std::string>> Permissions;
    std::string Owner;
    std::vector<std::string> Plugins;

    Config();
    bool Load(const std::string& jsonfile);
    void LoadShip(api::Ship ship);

    Json::Value GetRoot() const;
    Json::Value GetZoneConfig() const;
};

std::ostream& operator<<(std::ostream& os, const Config& config);

#endif
