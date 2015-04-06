#include "RotationStore.h"

#include "Common.h"
#include "Tokenizer.h"

#include <vector>
#include <string>

namespace Ships {

static std::vector<std::string> ships = { "Warbird", "Javelin", "Spider", "Levi", "Terrier", "Weasel", "Lancaster", "Shark" };


void RotationStore::LoadDefaults() {
    for (int ship = 0; ship < 8; ++ship) {
        for (int i = 0; i < 40; ++i)
            m_Rotations[ship][i] = Ships::Rotations[ship][i];
    }
}

RotationStore::RotationStore(Config& config) {
    memset(m_Rotations, 0, sizeof(u64) * 8 * 40);

    for (int ship = 0; ship < 8; ++ship) {
        if (config.ShipRotations[ship].size() != 40) {
            LoadDefaults();
            return;
        }

        for (size_t i = 0; i < config.ShipRotations[ship].size(); ++i)
            m_Rotations[ship][i] = config.ShipRotations[ship][i];
    }
}

int RotationStore::GetRotation(u64 pixval) {
    int closest_ship = 0;
    int closest_rot = 0;
    long long closest_diff = 999999999;


    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 40; j++) {
            long long diff = std::abs((signed long long)(m_Rotations[i][j] - pixval));

            if (diff < closest_diff) {
                closest_ship = i;
                closest_rot = j;
                closest_diff = diff;
            }
        }
    }
    return closest_rot;
}

// Defaults
static const u64 Rotations[8][40] =
{
    // hs warbird
    { 113585296, 115626944, 114371716, 110756200, 108455251, 106942269, 107528738,
    111006261, 109493802, 109166098, 105618426, 101613026, 99769543, 98122676,
    99242694, 99569863, 97796777, 98915522, 97736655, 100498408, 102200807,
    103706331, 104821966, 104622546, 104630241, 103517930, 102860749, 104902141,
    105562397, 108456504, 110557006, 111808621, 112466775, 111217761, 111350628,
    110562670, 110230612, 111615099, 113193891, 113981331 },
    // hs jav
    { 138094160, 135927909, 134810208, 134284917, 133496442, 131199608, 130671444,
    130408540, 130735449, 131456588, 132574270, 131392580, 132307499, 132898096,
    132568100, 130732318, 131650334, 132437797, 133292584, 133423394, 134082335,
    134342926, 135982073, 138149377, 139330537, 139202544, 140780288, 140845328,
    140517396, 139601199, 139601962, 139142177, 140456738, 142294322, 141374766,
    139868234, 139671882, 139278430, 138750802, 138290265 },
    // hs spid
    { 68606664, 68015288, 69062345, 68210372, 67751625, 67883721, 67946430,
    67618236, 68211138, 68868542, 68339897, 68278982, 67880138, 67225537,
    68209356, 67751621, 67947197, 68212433, 68740808, 68210886, 68210628,
    68211126, 67358144, 67880394, 68407757, 67161802, 68013249, 67622097,
    68411852, 69720774, 68868798, 68473026, 68079540, 67817399, 67624150,
    68212690, 67949259, 68211399, 68212934, 67490233 },
    // hs levi
    { 54529920, 54855024, 54396791, 54069643, 53609090, 54330503, 53871492,
    54462085, 53347458, 54133371, 54856571, 55314789, 55709805, 55774818,
    56105568, 56035661, 56496978, 56822598, 56100398, 56037426, 56758815,
    57151529, 56889373, 57937682, 58007069, 57612813, 58989842, 58006041,
    56955660, 56957727, 56824360, 56432692, 55903784, 55706922, 54789697,
    54590783, 53606475, 54394207, 53412959, 54329965 },
    // hs terrier
    { 119665249, 120521599, 121182071, 122105235, 120921221, 120857743, 120983939,
    119803263, 120526220, 119209835, 120064892, 117434469, 119538802, 121443964,
    121118092, 119214733, 121183636, 119474043, 119474810, 119802237, 119998061,
    120392828, 118356837, 120660359, 122431379, 120658822, 121249947, 121245569,
    120789124, 118618736, 120587900, 120591747, 120197245, 122101374, 122035605,
    122892710, 122171045, 121639048, 122101138, 121245822 },
    // hs weasel
    { 123966850, 124031356, 122649698, 122849142, 121599072, 122057825, 122782054,
    122782572, 123374708, 123769724, 123770241, 122653562, 123375488, 122586747,
    121796714, 120743522, 121992030, 122979180, 122648683, 123768959, 125804673,
    122587509, 123768447, 122193022, 121601662, 122779241, 122453100, 123111287,
    122587515, 123703690, 124951686, 124425865, 122651254, 122125165, 122191993,
    121070446, 121139546, 121335908, 122779494, 124161935 },
    // hs lanc
    { 106273416, 106406312, 105686708, 104834255, 104770767, 103452872, 103189180,
    103121318, 101084816, 101476225, 100686953, 101473888, 100357721, 99700837,
    99305029, 98577697, 98182939, 99299607, 98774028, 99428094, 99754986,
    101397487, 101461211, 101199070, 101000898, 101919428, 102184417, 103565553,
    102712558, 103960298, 104352725, 106259704, 106195457, 106592295, 106855210,
    106792007, 106925149, 107386483, 104826478, 106009723 },
    // hs shark
    { 91972158, 92828217, 96970858, 103475075, 106775540, 108225303, 112496225,
    112897671, 112498284, 113356167, 113630627, 115201442, 116776624, 118624228,
    119542763, 119995361, 120596734, 119806448, 119344090, 119076033, 115927720,
    119992752, 116052373, 115195776, 113615723, 112362816, 108218614, 105383848,
    99726204, 94277977, 91707442, 92370507, 91776310, 91582268, 91319869,
    92502081, 91714115, 91450701, 90721837, 91178024 }
};


} // ns
