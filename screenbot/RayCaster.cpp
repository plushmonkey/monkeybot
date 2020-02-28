#include "RayCaster.h"

#include "Level.h"
#include <algorithm>

double BoxPointDistance(Vec2 box_pos, Vec2 box_extent, Vec2 p) {
  Vec2 bmin = box_pos;
  Vec2 bmax = box_pos + box_extent;

  double dx = std::max(std::max(bmin.x - p.x, 0.0), p.x - bmax.x);
  double dy = std::max(std::max(bmin.y - p.y, 0.0), p.y - bmax.y);

  return std::sqrt(dx * dx + dy * dy);
}

bool RayBoxIntersect(Vec2 origin, Vec2 direction, Vec2 box_pos, Vec2 box_extent, float* dist, Vec2* norm) {
  Vec2 recip(1.0 / direction.x, 1.0 / direction.y);
  Vec2 lb = box_pos + Vec2(0, box_extent.y);
  Vec2 rt = box_pos + Vec2(box_extent.x, 0);

  float t1 = (float)((lb.x - origin.x) * recip.x);
  float t2 = (float)((rt.x - origin.x) * recip.x);
  float t3 = (float)((lb.y - origin.y) * recip.y);
  float t4 = (float)((rt.y - origin.y) * recip.y);

  using std::min;
  using std::max;

  float tmin = max(min(t1, t2), min(t3, t4));
  float tmax = min(max(t1, t2), max(t3, t4));

  bool intersected = false;
  float t;

  if (tmax < 0) {
    t = tmax;
  } else if (tmin > tmax) {
    t = tmax;
  } else {
    intersected = true;
    t = tmin;

    if (norm) {
      if (t == t1) {
        *norm = Vec2(-1, 0);
      } else if (t == t2) {
        *norm = Vec2(1, 0);
      } else if (t == t3) {
        *norm = Vec2(0, -1);
      } else if (t == t4) {
        *norm = Vec2(0, 1);
      } else {
        *norm = Vec2(0, 0);
      }
    }
  }

  if (dist) {
    *dist = t;
  }

  return intersected;
}

CastResult RayCast(const Level& level, Vec2 from, Vec2 direction, double max_length) {
  static const Vec2 kDirections[] = { Vec2(0, 0), Vec2(1, 0), Vec2(-1, 0), Vec2(0, 1), Vec2(0, -1) };

  CastResult result = { 0 };
  float closest_distance = std::numeric_limits<float>::max();
  Vec2 closest_normal;

  for (double i = 0; i < max_length; ++i) {
    Vec2 current = from + direction * i;

    for (Vec2 check_direction : kDirections) {
      Vec2 check = current + check_direction;

      if (!level.IsSolid((unsigned short)check.x, (unsigned short)check.y)) continue;

      float dist;
      Vec2 normal;

      if (RayBoxIntersect(from, direction, check, Vec2(1, 1), &dist, &normal)) {
        if (dist < closest_distance && dist >= 0) {
          closest_distance = dist;
          closest_normal = normal;
        }
      }
    }

    if (closest_distance < max_length) {
      result.hit = true;
      result.normal = closest_normal;
      result.position = from + direction * closest_distance;
    }
  }

  return result;
}