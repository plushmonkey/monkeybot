#pragma once

#ifndef SCREENBOT_JSON_JSON_H_
#define SCREENBOT_JSON_JSON_H_

#include "lib/json.hpp"

using json = nlohmann::json;

inline bool exists(const json& node, const char* key) {
  return node.find(key) != node.end();
}

#endif
