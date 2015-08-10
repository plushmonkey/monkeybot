#include "Selector.h"

#include "Player.h"
#include "Util.h"
#include "MemorySensor.h"
#include "Config.h"
#include "Bot.h"

bool InRect(Vec2 pos, Vec2 min_rect, Vec2 max_rect) {
    return ((pos.x >= min_rect.x && pos.y >= min_rect.y) &&
            (pos.x <= max_rect.x && pos.y <= max_rect.y));
}

std::shared_ptr<api::Player> ClosestEnemySelector::Select(api::Bot* bot) {
    ClientPtr client = bot->GetClient();

    std::vector<api::PlayerPtr> enemies = client->GetEnemies(); 

    if (enemies.size() == 0) return api::PlayerPtr(nullptr);

    // Distance of closest enemy with multiplier applied
    double closest_calc_dist = std::numeric_limits<double>::max();
    api::PlayerPtr closest = api::PlayerPtr(nullptr);
    // Determines how much the rotation difference will increase distance by
    // TODO: change this to calculate rotation time using client settings
    const double RotationMultiplier = 2.0; 

    Vec2 spawn(((Bot*)bot)->GetConfig().SpawnX, ((Bot*)bot)->GetConfig().SpawnY);
    int center_radius = ((Bot*)bot)->GetConfig().CenterRadius;
    bool center_only = ((Bot*)bot)->GetConfig().CenterOnly;
    Vec2 bot_pos = bot->GetPos();
    bool has_x = (bot->GetMemorySensor().GetBotPlayer()->GetStatus() & 4) != 0;
    Vec2 half_resolution = Vec2(1920, 1080) / 2;
    Vec2 view_min = bot_pos - half_resolution / 16;
    Vec2 view_max = bot_pos + half_resolution / 16;

    for (unsigned int i = 0; i < enemies.size(); i++) {
        api::PlayerPtr& enemy = enemies.at(i);
        int cdx, cdy;
        double cdist;

        Vec2 enemy_pos = enemy->GetPosition() / 16;

        if (enemy_pos.x <= 0 && enemy_pos.y <= 0) continue;
        if (client->IsInSafe(enemy_pos, bot->GetLevel())) continue;

        bool in_center = InRect(enemy_pos, spawn - center_radius, spawn + center_radius);

        if (center_only && !in_center) continue;

        Util::GetDistance(enemy_pos, bot_pos, &cdx, &cdy, &cdist);

        if (cdist < 15 && bot->IsInSafe()) continue;

        unsigned char status = enemy->GetStatus();
        bool stealth = (status & 1) != 0;
        bool cloak = (status & 2) != 0;

        if (!has_x) {
            if (stealth && cloak) continue;
            bool visible = InRect(enemy_pos, view_min, view_max);

            if (stealth && !visible) continue;
        }

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


TargetEnemySelector::TargetEnemySelector(const std::string& name) 
    : m_Target(Util::strtolower(name)) 
{ 
}

api::PlayerPtr TargetEnemySelector::Select(api::Bot* bot) {
   api::PlayerList players = bot->GetMemorySensor().GetPlayers();

    Vec2 bad_pos(0, 0);
    unsigned int bot_freq = bot->GetFreq();

    bool center_only = ((Bot*)bot)->GetConfig().CenterOnly;
    int center_radius = ((Bot*)bot)->GetConfig().CenterRadius;
    Vec2 spawn(((Bot*)bot)->GetConfig().SpawnX, ((Bot*)bot)->GetConfig().SpawnY);

   api::PlayerList::iterator find = std::find_if(players.begin(), players.end(), [&](api::PlayerPtr player) {
        std::string lower = Util::strtolower(player->GetName());
        Vec2 pos = player->GetPosition() / 16;

        bool in_safe = bot->GetClient()->IsInSafe(pos, bot->GetLevel());
        bool in_center = InRect(pos, spawn - center_radius, spawn + center_radius);
        bool can_target = !in_safe && (!center_only || in_center);

        return lower.compare(m_Target) == 0 && 
               player->GetShip() != api::Ship::Spectator &&
               player->GetPosition() / 16 != bad_pos &&
               player->GetFreq() != bot_freq &&
               can_target;
    });

    if (find != players.end())
        return *find;

    // Fall back to choosing closest enemy if target can't be found
    return m_ClosestSelector.Select(bot);
}

api::SelectorPtr EnemySelectorFactory::CreateClosest() {
    return api::SelectorPtr(new ClosestEnemySelector());
}

api::SelectorPtr EnemySelectorFactory::CreateTarget(const std::string& target) {
    return api::SelectorPtr(new TargetEnemySelector(target));
}
