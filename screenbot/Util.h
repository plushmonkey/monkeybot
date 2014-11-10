#ifndef UTIL_H_
#define UTIL_H_

#include "Common.h"

#include <vector>

class Level;

namespace Util {
    float GetRadarPerPixel(const ScreenAreaPtr& radar, int mapzoom);
    Coord GetBotRadarPos(Coord real_pos, const ScreenAreaPtr& radar, int mapzoom);
    int GetEnergy(ScreenAreaPtr* energyarea);
    int GetEnergyDigit(int digit, ScreenAreaPtr* energyarea);
    bool InSafe(const ScreenAreaPtr& area, Coord coord);
    void GetDistance(Coord from, Coord to, int *dx, int *dy, double* dist);
    int GetTargetRotation(int dx, int dy);
    bool XRadarOn(const ScreenGrabberPtr& grabber);
    int GetShip(const ScreenAreaPtr& ship);
    int GetShipRadius(int n);
    bool InShip(const ScreenGrabberPtr& grabber);
    bool IsClearPath(Coord from, Coord target, int radius, const Level& level);
    Coord FindTargetPos(Coord bot_pos, Coord radar_coord, const ScreenGrabberPtr& grabber, const ScreenAreaPtr& radar, const Level& level, int mapzoom);
}

#endif
