#include "api/Api.h"
#include "plugin/Plugin.h"
#include "Vector.h"

#undef max

#include <iostream>
#include <memory>

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

bool InRect(Vec2 pos, Vec2 min_rect, Vec2 max_rect) {
    return ((pos.x >= min_rect.x && pos.y >= min_rect.y) &&
            (pos.x <= max_rect.x && pos.y <= max_rect.y));
}

class HyperspaceSelector : public api::Selector {
public:
    std::shared_ptr<api::Player> Select(api::Bot* bot) {
        ClientPtr client = bot->GetClient();

        std::vector<api::PlayerPtr> enemies = client->GetEnemies();

        if (enemies.size() == 0) return api::PlayerPtr(nullptr);

        // Distance of closest enemy with multiplier applied
        double closest_calc_dist = std::numeric_limits<double>::max();
        api::PlayerPtr closest = api::PlayerPtr(nullptr);
        // Determines how much the rotation difference will increase distance by
        const double RotationMultiplier = 2.25;

        Vec2 bot_pos = bot->GetPos();
        bool has_x = (bot->GetMemorySensor().GetBotPlayer()->GetStatus() & 4) != 0;
        Vec2 half_resolution = Vec2(1920, 1080) / 2;
        Vec2 view_min = bot_pos - half_resolution / 16;
        Vec2 view_max = bot_pos + half_resolution / 16;

        Vec2 spawn(512, 512);
        const int center_radius = 512 - 320;

        for (unsigned int i = 0; i < enemies.size(); ++i) {
            api::PlayerPtr& enemy = enemies.at(i);
            Vec2 enemy_pos = enemy->GetPosition() / 16;
            
            if (enemy_pos.x <= 0 && enemy_pos.y <= 0) continue;
            if (client->IsInSafe(enemy_pos, bot->GetLevel())) continue;

            int cdx, cdy;
            double cdist;
            GetDistance(enemy_pos, bot_pos, &cdx, &cdy, &cdist);

            if (cdist < 15 && bot->IsInSafe()) continue;

            bool in_center = InRect(enemy_pos, spawn - center_radius, spawn + center_radius);
            bool in_mini = InRect(enemy_pos, Vec2(460, 613), Vec2(534, 670));
            if (!in_center || in_mini) continue;

            unsigned char status = enemy->GetStatus();
            bool stealth = (status & 1) != 0;
            bool cloak = (status & 2) != 0;

            if (!has_x) {
                if (stealth && cloak) continue;
                bool visible = InRect(enemy_pos, view_min, view_max);

                if (stealth && !visible) continue;
            }

            // Unit vector pointing towards this enemy
            Vec2 to_target = Vec2Normalize(enemy_pos - bot_pos);
            double dot = bot->GetHeading().Dot(to_target);
            double multiplier = 1.0 + ((1.0 - dot) / RotationMultiplier);
            double calc_dist = cdist * multiplier;

            if (calc_dist < closest_calc_dist) {
                closest_calc_dist = calc_dist;
                closest = enemy;
            }
        }

        return closest;
    }
};

class HyperspacePlugin : public Plugin {
private:
    api::Bot* m_Bot;
    std::shared_ptr<HyperspaceSelector> m_Selector;

public:
    HyperspacePlugin(api::Bot* bot)
        : m_Bot(bot),
          m_Selector(new HyperspaceSelector())
    {
    }

    int OnCreate() {
        return 0;
    }

    int OnUpdate(unsigned long dt) {
        m_Bot->SetEnemySelector(m_Selector);
        return 0;
    }

    int OnDestroy() {
        auto factory = m_Bot->GetEnemySelectors();
        m_Bot->SetEnemySelector(factory->CreateClosest());
        return 0;
    }

};

extern "C" {
    PLUGIN_FUNC Plugin* CreatePlugin(api::Bot* bot) {
        return new HyperspacePlugin(bot);
    }

    PLUGIN_FUNC void DestroyPlugin(Plugin* plugin) {
        delete plugin;
    }

    PLUGIN_FUNC const char* GetPluginName() {
        return "Hyperspace";
    }
}
