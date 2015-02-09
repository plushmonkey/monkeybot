#ifndef UTIL_H_
#define UTIL_H_

#include "Common.h"
#include "Pathing.h"

#include <vector>

class Level;

namespace Util {
    float GetRadarPerPixel(const ScreenAreaPtr& radar, int mapzoom);
    Vec2 GetBotRadarPos(Vec2 real_pos, const ScreenAreaPtr& radar, int mapzoom);
    int GetEnergy(ScreenAreaPtr* energyarea);
    int GetEnergyDigit(int digit, ScreenAreaPtr* energyarea);
    bool InSafe(const ScreenAreaPtr& area, Vec2 coord);
    void GetDistance(Vec2 from, Vec2 to, int *dx, int *dy, double* dist);
    int GetTargetRotation(int dx, int dy);
    int ContRotToDegrees(int rot);
    double ContRotToRads(int rot);
    Vec2 ContRotToVec(int rot);
    bool XRadarOn(const ScreenGrabberPtr& grabber);
    int GetShip(const ScreenAreaPtr& ship);
    int GetShipRadius(int n);
    bool InShip(const ScreenGrabberPtr& grabber);
    bool IsClearPath(Vec2 from, Vec2 target, int radius, const Level& level);
    Vec2 FindTargetPos(Vec2 bot_pos, Vec2 radar_coord, const ScreenGrabberPtr& grabber, const ScreenAreaPtr& radar, const Level& level, int mapzoom);
    Direction GetRotationDirection(Vec2 heading, Vec2 target);
    void str_replace(std::string& str, const std::string& from, const std::string& to);

    bool NearWall(Vec2 pos, Pathing::Grid<short>& grid);
}

#endif
