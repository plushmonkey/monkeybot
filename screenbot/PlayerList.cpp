#include "PlayerList.h"

#include "ScreenArea.h"
#include "Common.h"
#include "Font.h"
#include <iostream>

const Pixel BackgroundColor(10, 25, 10, 0);


PlayerList::PlayerList() { }

PlayerList::PlayerList(ScreenAreaPtr area)
    : m_Area(area)
{

}

void PlayerList::SetScreenArea(ScreenAreaPtr area) {
    m_Area = area;
    m_Area->Update();
}

void PlayerList::Update(unsigned long dt) {
    m_Area->Update();

    static DWORD plist_timer = 0;

    plist_timer += dt;

    if (plist_timer < 1000) return;

    plist_timer = 0;

    std::vector<std::string> freq;
    std::string cur_freq;
    std::string line;
    /* ugly code here for testing */
    for (int y = 16; ; y += 12) {
        line = "";
        for (int i = 2; i < m_Area->GetWidth() - 8; i += 8) {
            ScreenAreaPtr t = m_Area->GetArea(i, y, 7, 10);
            t->Update();

            std::vector<Pixel> ign = { BackgroundColor };
            char out;

            if (Fonts::TallFont->GetCharacter(t, ign, &out))
                line += out;
        }

        if (line.find("-------------") != std::string::npos) {
            if (freq.size() > 0)
                std::cout << cur_freq << ": ";
            for (size_t i = 0; i < freq.size(); ++i) {
                if (i != 0) 
                    std::cout << ", ";
                std::cout << freq.at(i);
            }
            std::cout << std::endl;
                
            cur_freq = line.substr(0, 4);
            freq.clear();
        } else {
            if (line.length() > 0)
                freq.push_back(line.substr(0, line.find("  ")));
        }

        if (line.length() == 0)
            break;
    }
    if (freq.size() > 0)
        std::cout << cur_freq << ": ";
    for (size_t i = 0; i < freq.size(); ++i) {
        if (i != 0) std::cout << ", ";
        std::cout << freq.at(i);
    }
    std::cout << std::endl;

    cur_freq = line.substr(0, 4);
    freq.clear();
}
