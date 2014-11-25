#ifndef CLIENT_H_
#define CLIENT_H_

#include "Common.h"
#include "Config.h"
#include "Keyboard.h"
#include "PlayerList.h"

#include <Windows.h>
#include <vector>

class Level;

enum class GunState {
    Tap,
    Constant,
    Off
};

class Client {
public:
    Client() { }
    virtual ~Client() { }

    virtual void Update(DWORD dt) = 0;

    virtual void Bomb() = 0;
    virtual void Gun(GunState state, int energy_percent = 100) = 0;
    virtual void Burst() = 0;
    virtual void Repel() = 0;

    virtual void SetXRadar(bool on) = 0;
    virtual void Warp() = 0;
    
    virtual int GetEnergy() = 0;
    virtual int GetRotation() = 0;
    virtual bool InSafe(Vec2 real_pos, const Level& level) = 0;

    virtual void Up(bool val) = 0;
    virtual void Down(bool val) = 0;
    virtual void Left(bool val) = 0;
    virtual void Right(bool val) = 0;

    virtual void Attach() = 0;

    virtual bool InShip() const = 0;
    virtual void EnterShip(int num) = 0;

    virtual std::vector<Vec2> GetEnemies(Vec2 real_pos, const Level& level) = 0;
    virtual Vec2 GetClosestEnemy(Vec2 real_pos, const Level& level, int* dx, int* dy, double* dist) = 0;

    virtual Vec2 GetRealPosition(Vec2 bot_pos, Vec2 target, const Level& level) = 0;

    virtual void ReleaseKeys() = 0;
    virtual void ToggleKeys() = 0;

    virtual std::string GetName() = 0;
    virtual int GetFreq() = 0;

    virtual PlayerList GetFreqPlayers(int freq) = 0;
    virtual PlayerList GetPlayers() = 0;

    virtual bool OnSoloFreq() = 0;
    virtual PlayerPtr GetSelectedPlayer() = 0;
    virtual void MoveTicker(bool down) = 0;
};

namespace Ships {
    class RotationStore;
}

class ScreenClient : public Client {
private:
    Config& m_Config;
    Keyboard m_Keyboard;
    HWND m_Window;

    ScreenGrabberPtr m_Screen;
    ScreenAreaPtr m_Radar;
    ScreenAreaPtr m_Ship;
    ScreenAreaPtr m_Player;
    ScreenAreaPtr m_EnergyArea[4];
    PlayerWindow m_PlayerWindow;

    DWORD m_LastBullet;
    DWORD m_LastBomb;

    bool m_ConfigLoaded;
    int m_MapZoom;

    bool m_FireBombs;
    bool m_FireGuns;
    bool m_ScaleDelay;

    int m_BulletDelay;
    int m_CurrentBulletDelay;
    int m_BombDelay;

    bool m_IgnoreCarriers;

    Ships::RotationStore* m_Rotations;

    void GrabRadar();
public:
    ScreenClient(HWND hwnd, Config& config);

    virtual void Update(DWORD dt);

    virtual void Bomb();
    virtual void Gun(GunState state, int energy_percent = 100);
    virtual void Burst();
    virtual void Repel();

    virtual void SetXRadar(bool on);
    virtual void Warp();

    virtual int GetEnergy();
    virtual int GetRotation();
    virtual bool InSafe(Vec2 real_pos, const Level& level);

    virtual void Up(bool val);
    virtual void Down(bool val);
    virtual void Left(bool val);
    virtual void Right(bool val);

    virtual void Attach();

    virtual bool InShip() const;
    virtual void EnterShip(int num);

    virtual std::vector<Vec2> GetEnemies(Vec2 real_pos, const Level& level);
    virtual Vec2 GetClosestEnemy(Vec2 real_pos, const Level& level, int* dx, int* dy, double* dist);

    virtual Vec2 GetRealPosition(Vec2 bot_pos, Vec2 target, const Level& level);

    virtual void ReleaseKeys();
    virtual void ToggleKeys();

    virtual std::string GetName();
    virtual int GetFreq();

    virtual PlayerList GetFreqPlayers(int freq);
    virtual PlayerList GetPlayers();

    virtual bool OnSoloFreq();
    virtual PlayerPtr GetSelectedPlayer();
    virtual void MoveTicker(bool down);

    ScreenAreaPtr GetRadar() { return m_Radar; }
};

#endif
