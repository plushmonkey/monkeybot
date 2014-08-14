#include "Common.h"
#include "ScreenArea.h"
#include "Keyboard.h"
#include "WindowFinder.h"
#include "Bot.h"
#include "ScreenGrabber.h"
#include "State.h"
#include "Util.h"
#include <thread>
#include <tchar.h>
#include <iostream>
#include <map>
#include <string>
#include <queue>

Bot::Bot(int ship)
    : m_Finder(_T("Continuum")),
    m_Window(0),
    m_Keyboard(),
    m_LastEnemy(timeGetTime()),
    m_State(new AggressiveState(*this)),
    m_ShipNum(ship),
    m_Target(0, 0),
    m_TargetInfo(0, 0, 0),
    m_EnemyTargetInfo(0, 0, 0),
    m_Energy(0),
    m_MaxEnergy(0)
{}

ScreenAreaPtr& Bot::GetRadar() {
    return m_Radar;
}

ScreenAreaPtr& Bot::GetShip() {
    return m_Ship;
}

ScreenAreaPtr& Bot::GetPlayer() {
    return m_Player;
}

std::shared_ptr<ScreenGrabber> Bot::GetGrabber() {
    return m_Grabber;
}

HWND Bot::SelectWindow() {
    const WindowFinder::Matches& matches = m_Finder.GetMatches();
    std::map<int, const WindowFinder::Match*> select_map;

    int i = 1;
    for (const WindowFinder::Match& kv : matches) {
        select_map[i] = &kv;
        tcout << i++ << " " << kv.second << " (" << kv.first << ")" << std::endl;
    }

    if (i == 1)
        throw std::runtime_error("No windows found.");

    tcout << "> ";

    tstring input;
    tcin >> input;

    int selection = _tstoi(input.c_str());

    if (selection < 1 || selection >= i)
        throw std::runtime_error("Error with window selection.");

    tcout << "Running bot on window " << select_map[selection]->second << "." << std::endl;

    return select_map[selection]->first;
}

void Bot::SelectShip() {
    static bool selected;

    if (selected == true) return;

    selected = true;
    tcout << "Ship number: ";

    std::string input;
    std::cin >> input;

    if (input.length() != 1 || input[0] < 0x31 || input[0] > 0x38) {
        tcout << "Defaulting to ship 8." << std::endl;
        return;
    }

    m_ShipNum = input[0] - 0x30;
}

std::vector<Coord> GetNeighbors(Coord pos, int rwidth) {
    std::vector<Coord> neighbors;
    neighbors.resize(4);
    int x, y;

    for (int i = 0; i < 4; i++)
        neighbors[i] = Coord(0, 0);

    x = pos.x;
    y = pos.y - 1;
    if (x > 0 && x < rwidth && y > 0 && y < rwidth)
        neighbors[0] = Coord(x, y);

    x = pos.x + 1;
    y = pos.y;
    if (x > 0 && x < rwidth && y > 0 && y < rwidth)
        neighbors[1] = Coord(x, y);

    x = pos.x;
    y = pos.y + 1;
    if (x > 0 && x < rwidth && y > 0 && y < rwidth)
        neighbors[2] = Coord(x, y);

    x = pos.x - 1;
    y = pos.y;
    if (x > 0 && x < rwidth && y > 0 && y < rwidth)
        neighbors[3] = Coord(x, y);

    return neighbors;
}

void Bot::Update() {
    m_Grabber->Update();
    m_Radar->Update();
    m_Ship->Update();
    m_Player->Update();

    if (!Util::InShip(m_Grabber)) {
        m_Keyboard.Up(VK_CONTROL);
        m_Keyboard.Send(VK_ESCAPE);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        m_Keyboard.Send(0x30 + m_ShipNum);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    m_Energy = Util::GetEnergy(GetEnergyAreas());

    if (m_Energy > m_MaxEnergy) m_MaxEnergy = m_Energy;

    static DWORD lastpf = 0;

    int rwidth = m_Radar->GetWidth();
    int dx, dy;
    double dist;

    try {
        std::vector<Coord> enemies = Util::GetEnemies(m_Radar);
        m_EnemyTarget = Util::GetClosestEnemy(enemies, m_Radar, &dx, &dy, &dist);

        m_EnemyTargetInfo.dx = dx;
        m_EnemyTargetInfo.dy = dy;
        m_EnemyTargetInfo.dist = dist;

        /*if (timeGetTime() > lastpf + 500) {
            Coord start(rwidth / 2, rwidth / 2);

            std::map<Coord, Coord> came_from;
            std::queue<Coord> frontier;

            frontier.push(start);
            came_from[start] = start;

            while (!frontier.empty()) {
                Coord current = frontier.front();
                frontier.pop();

                std::vector<Coord> neighbors = GetNeighbors(current, rwidth);

                for (Coord& coord : neighbors) {
                    if (m_Radar->GetPixel(coord.x, coord.y) != Colors::RadarWall) {
                        if (came_from.find(coord) == came_from.end()) {
                            frontier.push(coord);
                            came_from[coord] = current;
                        }
                    }
                }
            }

            m_Path.clear();

            Coord current = m_EnemyTarget;
            m_Path.push_back(current);
            while (current != start) {
                current = came_from[current];
                m_Path.push_back(current);
            }

            m_Target = m_EnemyTarget;
        //    if (m_Path.size() > 2) {
          //      m_Target = m_Path.at(m_Path.size() / 2);

             //   tcout << "tar: " << m_Target << std::endl;

                Util::GetDistance(m_Target, Coord(rwidth / 2, rwidth / 2), &dx, &dy, &dist);

                m_TargetInfo.dx = dx;
                m_TargetInfo.dy = dy;
                m_TargetInfo.dist = dist;
          //  } else {
            //    m_Target = m_EnemyTarget;
            //}

            lastpf = timeGetTime();
        }*/
    } catch (...) { 
        m_EnemyTarget = Coord(0, 0);
        m_Target = Coord(0, 0);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    m_State->Update();
}

void Bot::GrabRadar() {
    int radarstart = 0;
    int radarend = 0;
    int radary = 0;

    while (radarend == 0) {
        m_Grabber->Update();
        for (int y = static_cast<int>(m_Grabber->GetHeight() / 1.5); y < m_Grabber->GetHeight(); y++) {
            for (int x = static_cast<int>(m_Grabber->GetWidth() / 1.5); x < m_Grabber->GetWidth(); x++) {
                Pixel pix = m_Grabber->GetPixel(x, y);

                if (radarstart == 0 && pix == Colors::RadarBorder)
                    radarstart = x;

                if (radarstart != 0 && pix != Colors::RadarBorder) {
                    radarend = x - 1;
                    if (radarend - radarstart < 104) {
                        radarstart = 0;
                        radarend = 0;
                        break;
                    }
                    radarstart++;
                    radary = y + 1;
                    x = m_Grabber->GetWidth();
                    y = m_Grabber->GetHeight();
                    break;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
        tcout << "Finding radar location." << std::endl;
    }

    int radarwidth = radarend - radarstart;

    tcout << "Creating radar screen with width of " << radarwidth << " at " << radarstart << ", " << radary << "." << std::endl;

    m_Radar = m_Grabber->GetArea(radarstart, radary, radarwidth, radarwidth);

    if (!m_Radar.get()) {
        tcerr << "Resolution (" << m_Grabber->GetWidth() << "x" << m_Grabber->GetHeight() << ") not supported." << std::endl;
        std::abort();
    }
}

int Bot::Run() {
    bool ready = false;

    while (!ready) {
        try {
            if (!m_Window)
                m_Window = SelectWindow();

            SelectShip();

            m_Grabber = std::shared_ptr<ScreenGrabber>(new ScreenGrabber(m_Window));
            m_Ship = m_Grabber->GetArea(m_Grabber->GetWidth() / 2 - 18, m_Grabber->GetHeight() / 2 - 18, 36, 36);

            GrabRadar();

            m_Player = m_Radar->GetArea(m_Radar->GetWidth() / 2 - 1, m_Radar->GetWidth() / 2 - 1, 4, 4);

            int width = m_Grabber->GetWidth();

            m_EnergyArea[0] = m_Grabber->GetArea(width - 78, 0, 16, 21);
            m_EnergyArea[1] = m_Grabber->GetArea(width - 62, 0, 16, 21);
            m_EnergyArea[2] = m_Grabber->GetArea(width - 46, 0, 16, 21);
            m_EnergyArea[3] = m_Grabber->GetArea(width - 30, 0, 16, 21);

            ready = true;
        } catch (const std::exception& e) {
            tcerr << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }

    tcout << "Loading config file bot.conf" << std::endl;

    m_Config.Set(_T("XPercent"), _T("75"));
    m_Config.Set(_T("RunPercent"), _T("35"));
    m_Config.Set(_T("SafeResetTime"), _T("10000"));
    m_Config.Set(_T("TargetDistance"), _T("10"));
    m_Config.Set(_T("RunDistance"), _T("30"));
    m_Config.Set(_T("StopBombing"), _T("90"));
    m_Config.Set(_T("BombTime"), _T("5000"));
    m_Config.Set(_T("FireBombs"), _T("true"));
    m_Config.Set(_T("FireGuns"), _T("true"));
    m_Config.Set(_T("DistanceFactor"), _T("10"));

    if (!m_Config.Load(_T("bot.conf")))
        tcout << "Could not load bot.conf. Using default values." << std::endl;

    for (auto iter = m_Config.begin(); iter != m_Config.end(); ++iter)
        tcout << iter->first << ": " << iter->second << std::endl;

    while (true)
        Update();

    return 0;
}