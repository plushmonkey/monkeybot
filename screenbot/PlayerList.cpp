#include "PlayerList.h"

#include "ScreenArea.h"
#include "Common.h"
#include "Font.h"
#include <iostream>

namespace {

const Pixel BackgroundColor(10, 25, 10, 0);
const Pixel SelectedColor(99, 33, 0, 0);
const int PlayerListStart = 16; // pixels from top
const int LineHeight = 12; // pixels
const std::string FreqIndicator = "-------------";
const int UpdateFrequency = 150; // ms

}

PlayerWindow::PlayerWindow() { }

PlayerWindow::PlayerWindow(ScreenAreaPtr area)
    : m_Area(area), m_UpdateTimer(0)
{

}

void PlayerWindow::SetScreenArea(ScreenAreaPtr area) {
    m_Area = area;
    m_Area->Update();
}

std::string PlayerWindow::GetLineText(int line) {
    const int FontWidth = Fonts::TallFont->GetWidth();
    const int FontHeight = Fonts::TallFont->GetHeight();
    const int y = PlayerListStart + LineHeight * line;

    std::string text;

    for (int x = 2; x < m_Area->GetWidth() - FontWidth + 1; x += FontWidth + 1) {
        ScreenAreaPtr char_area = m_Area->GetArea(x, y, FontWidth, FontHeight);
        char_area->Update();

        std::vector<Pixel> ign = { BackgroundColor };
        char out;

        if (Fonts::TallFont->GetCharacter(char_area, ign, &out))
            text += out;
    }

    return text;
}

void PlayerWindow::Update(unsigned long dt) {
    m_Area->Update();

    m_UpdateTimer += dt;
    if (m_UpdateTimer < UpdateFrequency) return;
    m_UpdateTimer = 0;

    m_Players.clear();

    std::string line;

    int num_lines = (m_Area->GetHeight() - PlayerListStart) / LineHeight;
    int current_freq = 0;

    for (int i = 0; i < num_lines; ++i) {
        line = GetLineText(i);

        // No text on line so we hit the end of the window
        if (line.length() == 0) 
            break; 

        if (line.find(FreqIndicator) != std::string::npos) {
            // Freq found, update current_freq
            std::string freq_str = line.substr(0, 4);
            if (freq_str.find("-") != std::string::npos)
                current_freq = 9999;
            else
                current_freq = strtol(freq_str.c_str(), nullptr, 10);
        } else {
            // Player found, add them to the current freq
            std::string name = line;
            
            // Find and strip front spaces
            for (size_t n = 0; n < name.length(); ++n) {
                if (name[n] != ' ') {
                    name = name.substr(n, name.length() - n);
                    break;
                }
            }
            // Strip trailing spaces
            std::string::size_type end = name.find("  "); 
            name = name.substr(0, end);

            auto player = std::make_shared<Player>(name, current_freq);
            m_Players.emplace_back(player);

            // Check if this player is the selected player
            if (m_Area->GetPixel(0, 5 + PlayerListStart + i * LineHeight) == SelectedColor)
                m_Selected = player;
        }
    }
}

PlayerPtr PlayerWindow::Find(const std::string& name) {
    for (PlayerPtr player : m_Players) {
        if (player->GetName().compare(name) == 0) 
            return player;
    }

    return std::shared_ptr<Player>();
}

PlayerList PlayerWindow::GetFrequency(int freq) {
    PlayerList list;
    for (PlayerPtr player : m_Players) {
        if (player->GetFreq() == freq)
            list.push_back(player);
    }
    return list;
}
