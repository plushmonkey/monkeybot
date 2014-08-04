#include "Util.h"
#include "ScreenArea.h"
#include <limits>

namespace Util {

bool PlayerInSafe(const ScreenAreaPtr& player) {
    try {
        player->Find(Colors::SafeColor);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void GetClosestEnemy(const std::vector<Coord>& enemies, ScreenAreaPtr& radar, int* dx, int* dy, double* dist) {
    *dist = std::numeric_limits<double>::max();

    int radar_center = static_cast<int>(std::ceil(radar->GetWidth() / 2.0));

    for (unsigned int i = 0; i < enemies.size(); i++) {
        int cdx = enemies.at(i).x - radar_center;
        int cdy = enemies.at(i).y - radar_center;

        double cdist = std::sqrt(cdx * cdx + cdy * cdy);

        if (cdist < *dist) {
            *dist = cdist;
            *dx = cdx;
            *dy = cdy;
        }
    }
}
int GetTargetRotation(int dx, int dy) {
    double angle = std::atan2(dy, dx) * 180 / M_PI;
    int target = static_cast<int>(angle / 9) + 10;
    if (target < 0) target += 40;
    return target;
}

int GetRotation(const ScreenAreaPtr& area) {
    u64 val = 0;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++)
            val = area->GetPixel(16 + j, 16 + i) + val;
    }

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 40; j++)
            if (Ships::Rotations[i][j] == val) return j;
    }

    return -1;
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
            if (pix == Colors::EnemyColor || pix == Colors::EnemyBallColor) {
                int count = 0;
                try {
                    Pixel pixel;

                    // right
                    pixel = radar->GetPixel(x + 1, y);
                    if (pixel == Colors::EnemyColor || pixel == Colors::EnemyBallColor)
                        count++;
                    // bottom-right
                    pixel = radar->GetPixel(x + 1, y + 1);
                    if (pixel == Colors::EnemyColor || pixel == Colors::EnemyBallColor)
                        count++;
                    // bottom
                    pixel = radar->GetPixel(x, y + 1);
                    if (pixel == Colors::EnemyColor || pixel == Colors::EnemyBallColor)
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

}