#include "Util.h"
#include "ScreenArea.h"
#include "ScreenGrabber.h"
#include <limits>
#include <iostream>
#include <map>
#include "Level.h"

static const int ShipRadius[8] = { 15, 15, 30, 30, 21, 21, 39 ,11 };

namespace Util {

Coord FindTargetPos(Coord bot_pos, Coord radar_coord, const ScreenGrabberPtr& screen, const ScreenAreaPtr& radar, const Level& level) {
    float guess = 2.0f;
    if (radar->GetWidth() == 132) guess = 1.6f;
    int width = radar->GetWidth();

    int rdx = -((width / 2) - radar_coord.x);
    int rdy = -((width / 2) - radar_coord.y);

    Coord target(static_cast<int>(bot_pos.x + rdx * guess), static_cast<int>(bot_pos.y + rdy * guess));

    if (level.IsSolid(target.x, target.y)) {
        static std::vector<Coord> directions = {
                { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 },
                { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 }
        };
        for (const Coord& dir : directions) {
            if (!level.IsSolid(target.x + dir.x, target.y + dir.y)) {
                target = Coord(target.x + dir.x, target.y + dir.y);
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

    /* Only handle x radar if the ship has x radar */
    return true;
}

bool PlayerInSafe(const ScreenAreaPtr& player) {
    try {
        player->Find(Colors::SafeColor);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool InShip(const ScreenGrabberPtr& grabber) {
    int x = grabber->GetWidth() - 8;
    int y = 25;

    return grabber->GetPixel(x, y) == Pixel(99, 90, 148, 0);
}

void GetDistance(Coord from, Coord to, int *dx, int *dy, double* dist) {
    *dx = from.x - to.x;
    *dy = from.y - to.y;

    *dist = std::sqrt(*dx * *dx + *dy * *dy);
}

Coord GetClosestEnemy(const std::vector<Coord>& enemies, ScreenAreaPtr& radar, int* dx, int* dy, double* dist) {
    *dist = std::numeric_limits<double>::max();
    Coord closest = enemies.at(0);
    int radar_center = static_cast<int>(std::ceil(radar->GetWidth() / 2.0));

    for (unsigned int i = 0; i < enemies.size(); i++) {
        int cdx, cdy;
        double cdist;

        GetDistance(enemies.at(i), Coord(radar_center, radar_center), &cdx, &cdy, &cdist);

        if (cdist < *dist) {
            *dist = cdist;
            *dx = cdx;
            *dy = cdy;
            closest = enemies.at(i);
        }
    }

    return closest;
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

bool InSafe(const ScreenArea::Ptr& area, Coord coord) {
    int x = coord.x;
    int y = coord.y;

    try {
        Pixel pixel;

        // up-left
        pixel = area->GetPixel(x - 1, y - 1);
        if (pixel == Colors::SafeColor)
            return true;
        // up
        pixel = area->GetPixel(x, y - 1);
        if (pixel == Colors::SafeColor)
            return true;
        // up-right
        pixel = area->GetPixel(x + 1, y - 1);
        if (pixel == Colors::SafeColor)
            return true;
        // right
        pixel = area->GetPixel(x + 1, y);
        if (pixel == Colors::SafeColor)
            return true;
        // left
        pixel = area->GetPixel(x - 1, y);
        if (pixel == Colors::SafeColor)
            return true;
        // bottom-left
        pixel = area->GetPixel(x - 1, y + 1);
        if (pixel == Colors::SafeColor)
            return true;
        // bottom-right
        pixel = area->GetPixel(x + 1, y + 1);
        if (pixel == Colors::SafeColor)
            return true;
        // bottom
        pixel = area->GetPixel(x, y + 1);
        if (pixel == Colors::SafeColor)
            return true;
    } catch (std::exception&) {}

    return false;
}

std::vector<Coord> GetEnemies(ScreenArea::Ptr& radar) {
    std::vector<Coord> enemies;

    for (int y = 0; y < radar->GetWidth(); y++) {
        for (int x = 0; x < radar->GetWidth(); x++) {
            Pixel pix = radar->GetPixel(x, y);
            if (pix == Colors::EnemyColor[0] || pix == Colors::EnemyColor[1] || pix == Colors::EnemyBallColor) {
                int count = 0;
                try {
                    Pixel pixel;

                    // right
                    pixel = radar->GetPixel(x + 1, y);
                    if (pixel == Colors::EnemyColor[0] || pixel == Colors::EnemyColor[1] || pixel == Colors::EnemyBallColor)
                        count++;
                    // bottom-right
                    pixel = radar->GetPixel(x + 1, y + 1);
                    if (pixel == Colors::EnemyColor[0] || pixel == Colors::EnemyColor[1] || pixel == Colors::EnemyBallColor)
                        count++;
                    // bottom
                    pixel = radar->GetPixel(x, y + 1);
                    if (pixel == Colors::EnemyColor[0] || pixel == Colors::EnemyColor[1] || pixel == Colors::EnemyBallColor)
                        count++;
                } catch (std::exception&) {}

                if (count >= 3) {
                    Coord coord(x, y);
                    if (!InSafe(radar, coord))
                        enemies.push_back(coord);
                }
            }
        }
    }

    if (enemies.size() == 0)
        throw std::runtime_error("No enemies near.");
    return enemies;
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

    for (int i = 0; i < 4; i++)
        energyarea[i]->Update();

    int first = GetEnergyDigit(0, energyarea);
    int second = GetEnergyDigit(1, energyarea);
    int third = GetEnergyDigit(2, energyarea);
    int fourth = GetEnergyDigit(3, energyarea);

    return (first * 1000) + (second * 100) + (third * 10) + fourth;
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

bool IsClearPath(Coord from, Coord target, int radius, const Level& level) {
    const int PathClearIncrease = 8;
    int numpixels;
    int d, dinc1, dinc2;
    int x, xinc1, xinc2;
    int y, yinc1, yinc2;

    from.x *= 16;
    from.y *= 16;
    target.x *= 16;
    target.y *= 16;

    int dx = target.x - from.x;
    int dy = target.y - from.y;

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

    x = from.x;
    y = from.y;

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