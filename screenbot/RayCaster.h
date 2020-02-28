#pragma once
#ifndef MONKEYBOT_RAYCASTER_H_
#define MONKEYBOT_RAYCASTER_H_

#include "Vector.h"

struct CastResult {
  bool hit;
  Vec2 position;
  Vec2 normal;
};

class Level;

bool RayBoxIntersect(Vec2 origin, Vec2 direction, Vec2 box_pos, Vec2 box_extent, float* dist, Vec2* norm);
double BoxPointDistance(Vec2 box_pos, Vec2 box_extent, Vec2 point);

CastResult RayCast(const Level& level, Vec2 from, Vec2 direction, double max_length);

#endif
