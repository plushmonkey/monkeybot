#include "Util.h"

#include "ScreenArea.h"
#include "ScreenGrabber.h"
#include "Level.h"

#include <limits>
#include <iostream>
#include <map>

static const int ShipRadius[8] = { 15, 15, 30, 30, 21, 21, 39 ,11 };

namespace Util {

static const std::vector<Vec2> directions = {
        { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 },
        { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 }
};


void str_replace(std::string& str, const std::string& from, const std::string& to) {
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.length(), to);
        pos += to.length();
    }
}

bool NearWall(Vec2 pos, Pathing::Grid<short>& grid) {
    const int SearchLength = 2;

    for (Vec2 dir : directions) {
        for (int i = 0; i < SearchLength; ++i) {

            if (grid.IsSolid(pos.x + (dir.x * i), pos.y + (dir.y * i)))
                return true;
        }
    }

    return false;
}

float GetRadarPerPixel(const ScreenAreaPtr& radar, int mapzoom) {
    int rwidth = radar->GetWidth();
    float amountseen = (1024.0f / 2.0f) / mapzoom;
    return rwidth / amountseen;
}

int ContRotToDegrees(int rot) {
    return (40 - ((rot + 30) % 40)) * 9;
}

double ContRotToRads(int rot) {
    return (((40 - (rot + 30)) % 40) * 9) * (M_PI / 180);
}

Vec2 ContRotToVec(int rot) {
    double rad = Util::ContRotToRads(rot);
    return Vec2(std::cos(rad), -std::sin(rad));
}

Direction GetRotationDirection(Vec2 heading, Vec2 target) {
    Vec2 perp = heading.Perpendicular();
    target.Normalize();
    return perp.Dot(target) >= 0.0 ? Direction::Right : Direction::Left;
}

Vec2 GetBotRadarPos(Vec2 real_pos, const ScreenAreaPtr& radar, int mapzoom) {
    float per_pix = GetRadarPerPixel(radar, mapzoom);
    int rwidth = radar->GetWidth();

    double bot_radar_x = rwidth / 2.0f;
    double bot_radar_y = rwidth / 2.0f;

    float min_pos = (rwidth / 2) * per_pix;

    if (real_pos.x < min_pos)
        bot_radar_x *= real_pos.x / min_pos;
    else if (real_pos.x > 1024 - min_pos) // 961
        bot_radar_x += ((real_pos.x - (1024 - min_pos)) / per_pix);

    if (real_pos.y < min_pos)
        bot_radar_y *= real_pos.y / min_pos;
    else if (real_pos.y > 1024 - min_pos)
        bot_radar_y += ((real_pos.y - (1024 - min_pos)) / per_pix);

    bot_radar_x = std::max(std::min(bot_radar_x, rwidth - 1.0), 0.0);
    bot_radar_y = std::max(std::min(bot_radar_y, rwidth - 1.0), 0.0);

    return Vec2(bot_radar_x, bot_radar_y);
}

Vec2 FindTargetPos(Vec2 bot_pos, Vec2 radar_coord, const ScreenGrabberPtr& screen, const ScreenAreaPtr& radar, const Level& level, int mapzoom) {
    float per_pix = GetRadarPerPixel(radar, mapzoom);
    int rwidth = radar->GetWidth();

    Vec2 rpos = GetBotRadarPos(bot_pos, radar, mapzoom);

    double rdx = -(rpos.x - radar_coord.x);
    double rdy = -(rpos.y - radar_coord.y);

    Vec2 target(bot_pos.x + rdx * per_pix, bot_pos.y + rdy * per_pix);

    if (level.IsSolid((int)target.x, (int)target.y)) {
        for (const Vec2& dir : directions) {
            if (!level.IsSolid((int)(target.x + dir.x), (int)(target.y + dir.y))) {
                target = Vec2(target.x + dir.x, target.y + dir.y);
                break;
            }
        }
    }

    return target;
}

bool XRadarOn(const ScreenGrabberPtr& grabber) {
    int x = grabber->GetWidth() - 8;
    int y = grabber->GetHeight() == 600 ? 363 : 303;
    
    Pixel pixel = grabber->GetPixel(x, y);

    if (pixel == Colors::XRadarOff) return false;
    if (pixel == Colors::XRadarOn) return true;
    /* Only handle x radar if the ship has x radar */
    throw std::runtime_error("");
}

bool InShip(const ScreenGrabberPtr& grabber) {
    int x = grabber->GetWidth() - 8;
    int y = 25;

    return grabber->GetPixel(x, y) == Pixel(99, 90, 148, 0);
}

void GetDistance(Vec2 from, Vec2 to, int *dx, int *dy, double* dist) {
    double cdx = -(from.x - to.x);
    double cdy = -(from.y - to.y);
    if (dx)
        *dx = (int)cdx;
    if (dy)
        *dy = (int)cdy;
    if (dist)
        *dist = std::sqrt(cdx * cdx + cdy * cdy);
}

int GetTargetRotation(int dx, int dy) {
    double angle = std::atan2(dy, dx) * 180 / M_PI;
    int target = static_cast<int>(angle / 9) + 10;
    if (target < 0) target += 40;
    return target;
}

int GetShip(const ScreenAreaPtr& ship) {
    u64 val = 0;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++)
            val = ship->GetPixel(16 + j, 16 + i) + val;
    }

    for(int i = 0; i < 8; i++) {
        for (int j = 0; j < 40; j++)
            if (Ships::Rotations[i][j] == val) return i + 1;
    }

    return 1;
}

int GetShipRadius(int n) {
    return ShipRadius[n - 1];
}

bool InSafe(const ScreenArea::Ptr& area, Vec2 coord) {
    double x = coord.x;
    double y = coord.y;

    for (const Vec2& dir : directions) {
        try {
            Pixel pixel = area->GetPixel((int)(x + dir.x), (int)(y + dir.y));
            if (pixel == Colors::SafeColor)
                return true;
        } catch (...) {}
    }

    return false;
}

int GetEnergyDigit(int digit, ScreenAreaPtr* energyarea) {
    Pixel toppix = energyarea[digit]->GetPixel(8, 1);
    Pixel toprightpix = energyarea[digit]->GetPixel(13, 4);
    Pixel topleftpix = energyarea[digit]->GetPixel(4, 4);
    Pixel middlepix = energyarea[digit]->GetPixel(7, 10);
    Pixel bottomleftpix = energyarea[digit]->GetPixel(1, 16);
    Pixel bottomrightpix = energyarea[digit]->GetPixel(10, 15);
    Pixel bottompix = energyarea[digit]->GetPixel(5, 19);

    bool top = toppix == Colors::EnergyColor[0] || toppix == Colors::EnergyColor[1];
    bool topright = toprightpix == Colors::EnergyColor[0] || toprightpix == Colors::EnergyColor[1];
    bool topleft = topleftpix == Colors::EnergyColor[0] || topleftpix == Colors::EnergyColor[1];
    bool middle = middlepix == Colors::EnergyColor[0] || middlepix == Colors::EnergyColor[1];
    bool bottomleft = bottomleftpix == Colors::EnergyColor[0] || bottomleftpix == Colors::EnergyColor[1];
    bool bottomright = bottomrightpix == Colors::EnergyColor[0] || bottomrightpix == Colors::EnergyColor[1];
    bool bottom = bottompix == Colors::EnergyColor[0] || bottompix == Colors::EnergyColor[1];

    if (top && topright && topleft && !middle && bottomleft && bottomright && bottom)
        return 0;

    if (!top && topright && !topleft && !middle && !bottomleft && bottomright && !bottom)
        return 1;

    if (top && topright && !topleft && middle && bottomleft && !bottomright && bottom)
        return 2;

    if (top && topright && !topleft && middle && !bottomleft && bottomright && bottom)
        return 3;

    if (!top && topright && topleft && middle && !bottomleft && bottomright && !bottom)
        return 4;

    if (top && !topright && topleft && middle && !bottomleft && bottomright && bottom)
        return 5;

    if (top && !topright && topleft && middle && bottomleft && bottomright && bottom)
        return 6;

    if (top && topright && !topleft && !middle && !bottomleft && bottomright && !bottom)
        return 7;

    if (top && topright && topleft && middle && bottomleft && bottomright && bottom)
        return 8;

    if (top && topright && topleft && middle && !bottomleft && bottomright && bottom)
        return 9;

    return 0;
}

int GetEnergy(ScreenAreaPtr* energyarea) {
    // Default to 2000 energy if resolution isn't supported
    if (!energyarea[0]) return 2000;

    for (int i = 0; i < 5; i++)
        energyarea[i]->Update();

    int first = GetEnergyDigit(0, energyarea);
    int second = GetEnergyDigit(1, energyarea);
    int third = GetEnergyDigit(2, energyarea);
    int fourth = GetEnergyDigit(3, energyarea);
    int fifth = GetEnergyDigit(4, energyarea);

    return (first * 10000) + (second * 1000) + (third * 100) + (fourth * 10) + fifth;
}

bool FitsOnMap(int x, int y, int radius, const Level& level) {
    int startTileX = (x - radius) >> 4;
    int endTileX = (x + radius) >> 4;

    int startTileY = (y - radius) >> 4;
    int endTileY = (y + radius) >> 4;

    for (int x_ = startTileX; x_ <= endTileX; ++x_) {
        for (int y_ = startTileY; y_ <= endTileY; ++y_) {
            if (level.IsSolid(x_, y_))
                return false;
        }
    }

    return true;
}

bool IsClearPath(Vec2 from, Vec2 target, int radius, const Level& level) {
    const int PathClearIncrease = 8;
    int numpixels;
    int d, dinc1, dinc2;
    int x, xinc1, xinc2;
    int y, yinc1, yinc2;

    from.x *= 16;
    from.y *= 16;
    target.x *= 16;
    target.y *= 16;

    int dx = (int)(target.x - from.x);
    int dy = (int)(target.y - from.y);

    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    if (dx > dy) {
        numpixels = dx + 1;
        d = (2 * dy) - dx;
        dinc1 = dy << 1;
        dinc2 = (dy - dx) << 1;
        xinc1 = 1;
        xinc2 = 1;
        yinc1 = 0;
        yinc2 = 1;
    } else {
        numpixels = dy + 1;
        d = (2 * dx) - dy;
        dinc1 = dx << 1;
        dinc2 = (dx - dy) << 1;
        xinc1 = 0;
        xinc2 = 1;
        yinc1 = 1;
        yinc2 = 1;
    }

    if (from.x > target.x) {
        xinc1 = -xinc1;
        xinc2 = -xinc2;
    }
    if (from.y > target.y) {
        yinc1 = -yinc1;
        yinc2 = -yinc2;
    }

    dinc1 *= PathClearIncrease;
    dinc2 *= PathClearIncrease;
    xinc1 *= PathClearIncrease;
    xinc2 *= PathClearIncrease;
    yinc1 *= PathClearIncrease;
    yinc2 *= PathClearIncrease;

    x = (int)from.x;
    y = (int)from.y;

    for (int i = 1; i < numpixels; i += PathClearIncrease) {
        if (!FitsOnMap(x, y, radius, level))
            return false;

        if (d < 0) {
            d += dinc1;
            x += xinc1;
            y += yinc1;
        } else {
            d += dinc2;
            x += xinc2;
            y += yinc2;
        }
    }

    return true;
}


}