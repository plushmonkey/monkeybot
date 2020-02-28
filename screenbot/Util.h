#ifndef UTIL_H_
#define UTIL_H_

#include "Common.h"
#include "Pathing.h"

#include <vector>

class Level;

namespace Util {

    enum class Indicator {
        Gun, Bomb, Cloak, Stealth, XRadar, Antiwarp
    };

    float GetRadarPerPixel(const ScreenAreaPtr& radar, int mapzoom);
    Vec2 GetBotRadarPos(Vec2 real_pos, const ScreenAreaPtr& radar, int mapzoom);
    int GetEnergy(ScreenAreaPtr* energyarea, int digits);
    int GetEnergyDigit(int digit, ScreenAreaPtr* energyarea);
    bool InSafe(const ScreenAreaPtr& area, Vec2 coord);
    void GetDistance(Vec2 from, Vec2 to, int *dx, int *dy, double* dist);
    int GetTargetRotation(int dx, int dy);
    int ContRotToDegrees(int rot);
    double ContRotToRads(int rot);
    Vec2 ContRotToVec(int rot);
    bool XRadarOn(const ScreenGrabberPtr& grabber);
    int GetShipRadius(int n);
    bool InShip(const ScreenGrabberPtr& grabber);
    Vec2 TraceVector(Vec2 from, Vec2 target, int radius, const Level& level);
    bool IsClearPath(Vec2 from, Vec2 target, int radius, const Level& level);
    Vec2 FindTargetPos(Vec2 bot_pos, Vec2 radar_coord, const ScreenGrabberPtr& grabber, const ScreenAreaPtr& radar, const Level& level, int mapzoom);
    Direction GetRotationDirection(Vec2 heading, Vec2 target);
    void str_replace(std::string& str, const std::string& from, const std::string& to);

    bool NearWall(Vec2 pos, Pathing::Grid<short>& grid, int SearchLength = 2);

    inline std::string strtolower(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), tolower);
        return str;
    }

    int GetIndicatorTop(Indicator indicator, int screen_height);

    bool strtobool(const std::string& str);

    void ExitWithError(const std::string& error, bool pause = true);
}

#endif
