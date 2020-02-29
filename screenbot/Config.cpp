#include "Config.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <regex>

namespace {

const std::vector<std::string> ShipNames = {"Warbird",   "Javelin", "Spider",
                                            "Leviathan", "Terrier", "Weasel",
                                            "Lancaster", "Shark"};

std::regex VecRE(R"::(\(([0-9]+),\s*?([0-9]+)\))::");

}  // namespace

Config::Config()
    : XPercent(75),
      RunPercent(30),
      TargetDistance(10),
      RunDistance(30),
      StopBombing(90),
      BombDelay(500),
      FireBombs(true),
      FireGuns(true),
      Level(""),
      BulletDelay(20),
      ScaleDelay(true),
      CenterOnly(true),
      Patrol(true),
      Attach(false),
      MinGunRange(0),
      SpawnX(512),
      SpawnY(512),
      Baseduel(false),
      CenterRadius(250),
      IgnoreDelayDistance(10),
      RepelPercent(25),
      UseBurst(true),
      DecoyDelay(0),
      Taunt(false),
      TauntCooldown(6000),
      Hyperspace(true),
      Commander(false),
      Survivor(false),
      MultiFire(true),
      Revenge(false),
      Zone("Hyperspace"),
      Owner("") {}

void Config::LoadFromNode(const json& root) {
#define NODELOAD(x, type) \
  if (exists(root, #x)) x = root[#x].get<type>()

  NODELOAD(XPercent, int);
  NODELOAD(RunPercent, int);
  NODELOAD(TargetDistance, int);
  NODELOAD(RunDistance, int);
  NODELOAD(StopBombing, int);
  NODELOAD(BombDelay, int);
  NODELOAD(FireBombs, bool);
  NODELOAD(FireGuns, bool);
  NODELOAD(BulletDelay, int);
  NODELOAD(ScaleDelay, bool);
  NODELOAD(CenterOnly, bool);
  NODELOAD(Patrol, bool);
  NODELOAD(Attach, bool);
  NODELOAD(MinGunRange, int);
  NODELOAD(SpawnX, int);
  NODELOAD(SpawnY, int);
  NODELOAD(Baseduel, bool);
  NODELOAD(CenterRadius, int);
  NODELOAD(IgnoreDelayDistance, int);
  NODELOAD(RepelPercent, int);
  NODELOAD(UseBurst, bool);
  NODELOAD(DecoyDelay, int);
  NODELOAD(Taunt, bool);
  NODELOAD(TauntCooldown, int);
  NODELOAD(Hyperspace, bool);
  NODELOAD(Commander, bool);
  NODELOAD(Survivor, bool);
  NODELOAD(MultiFire, bool);
  NODELOAD(Revenge, bool);

  if (exists(root, "Level")) {
    Level = root["Level"].get<std::string>();
  }

  if (exists(root, "Owner")) {
    Owner = root["Owner"].get<std::string>();
  }

  if (exists(root, "Waypoints")) {
    Waypoints.clear();
    json wp_nodes = root.value("Waypoints", json());

    for (json wp_node : wp_nodes) {
      std::string waypoint = wp_node.get<std::string>();

      std::sregex_iterator begin(waypoint.begin(), waypoint.end(), VecRE);
      std::sregex_iterator end;

      if (begin == end) continue;
      std::smatch match = *begin;

      int x = atoi(std::string(match[1]).c_str());
      int y = atoi(std::string(match[2]).c_str());

      Waypoints.push_back(Vec2(x, y));
    }
  }

  if (exists(root, "Taunts")) {
    Taunts.clear();

    json taunt_nodes = root["Taunts"];

    for (json taunt_node : taunt_nodes) {
      Taunts.push_back(taunt_node.get<std::string>());
    }
  }

  if (exists(root, "TauntWhitelist")) {
    json whitelist_node = root["TauntWhitelist"];

    TauntWhitelist.clear();

    for (json name_node : whitelist_node) {
      std::string name = name_node.get<std::string>();

      std::transform(name.begin(), name.end(), name.begin(), tolower);
      TauntWhitelist.push_back(name);
    }
  }

  if (exists(root, "Permissions")) {
    json permissions_node = root["Permissions"];

    Permissions.clear();

    for (json::iterator it = permissions_node.begin();
         it != permissions_node.end(); ++it) {
      std::string name = it.key();

      for (json perm_node : *it) {
        Permissions[name].push_back(perm_node.get<std::string>());
      }
    }
  }

  if (exists(root, "Plugins")) {
    json plugins_node = root["Plugins"];

    Plugins.clear();

    for (json plugin_node : plugins_node) {
      Plugins.push_back(plugin_node.get<std::string>());
    }
  }
}

void Config::LoadShip(api::Ship ship) {
  std::string name = ShipNames[(int)ship];

  if (exists(root_, Zone.c_str())) {
    json zone_node = root_.value(Zone, json());

    if (exists(zone_node, name.c_str())) {
      LoadFromNode(zone_node.value(name.c_str(), json()));
    }
  }
}

json Config::GetRoot() const { return root_; }

json Config::GetZoneConfig() const {
  std::string zone = root_.value(Zone, "Hyperspace");

  return root_.value(zone, json());
}

std::string ReadAndStripComments(std::ifstream& input) {
  input.seekg(0, std::ios::end);
  std::ifstream::pos_type file_size = input.tellg();
  input.seekg(0, std::ios::beg);

  std::string data;

  data.resize(static_cast<std::size_t>(file_size));

  input.read(&data[0], file_size);
  input.close();

  std::string result;
  result.reserve(static_cast<std::size_t>(file_size));

  bool parsing_comment = false;

  for (std::size_t i = 0; i < data.size(); ++i) {
    char c = data[i];

    if (parsing_comment && i >= data.size() - 1) break;

    char next_c = data[i + 1];

    if (parsing_comment && c == '*' && next_c == '/') {
      parsing_comment = false;
      ++i;
      continue;
    }

    if (!parsing_comment && c == '/' && next_c == '*') {
      parsing_comment = true;
      ++i;
      continue;
    }

    if (!parsing_comment) {
      result.append(&c, 1);
    }
  }

  return result;
}

bool Config::Load(const std::string& jsonfile) {
  std::ifstream input(jsonfile, std::ios::binary);

  if (!input.is_open()) return false;

  std::string input_data = ReadAndStripComments(input);

  try {
    root_ = json::parse(input_data);
  } catch (json::parse_error& e) {
    std::cerr << e.what() << std::endl;
    return false;
  }

  Zone = root_.value("Zone", "Hyperspace");

  json zone_node = root_.value(Zone, json());

  if (!zone_node.is_null()) {
    LoadFromNode(zone_node);
  } else {
    std::cerr << "Zone " << Zone << " not found in config." << std::endl;
    return false;
  }

  return true;
}

std::ostream& operator<<(std::ostream& os, const Config& c) {
#define OUTPUT(x) os << #x << ": " << std::boolalpha << c.x << std::endl

  OUTPUT(XPercent);
  OUTPUT(RunPercent);
  OUTPUT(TargetDistance);
  OUTPUT(RunDistance);
  OUTPUT(StopBombing);
  OUTPUT(BombDelay);
  OUTPUT(FireBombs);
  OUTPUT(FireGuns);
  OUTPUT(Level);
  OUTPUT(BulletDelay);
  OUTPUT(ScaleDelay);
  OUTPUT(CenterOnly);
  OUTPUT(Patrol);
  OUTPUT(Attach);
  OUTPUT(MinGunRange);
  OUTPUT(SpawnX);
  OUTPUT(SpawnY);
  OUTPUT(Baseduel);
  OUTPUT(CenterRadius);
  OUTPUT(IgnoreDelayDistance);
  OUTPUT(RepelPercent);
  OUTPUT(UseBurst);
  OUTPUT(DecoyDelay);
  OUTPUT(Taunt);
  OUTPUT(TauntCooldown);
  OUTPUT(Hyperspace);
  OUTPUT(Commander);
  OUTPUT(Survivor);
  OUTPUT(MultiFire);
  OUTPUT(Revenge);
  OUTPUT(Zone);
  OUTPUT(Owner);

  os << "Waypoints: ";
  for (size_t i = 0; i < c.Waypoints.size(); ++i) {
    if (i != 0) os << ", ";
    os << c.Waypoints[i];
  }
  os << std::endl;

  os << "Taunt count: " << c.Taunts.size() << std::endl;
  os << "Taunt whitelist count: " << c.TauntWhitelist.size() << std::endl;

  os << "Plugins: ";

  for (auto iter = c.Plugins.begin(); iter != c.Plugins.end(); ++iter) {
    if (iter != c.Plugins.begin()) os << ", ";
    os << *iter;
  }
  os << std::endl;

  return os;
}
