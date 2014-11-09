#ifndef CLIENT_H_
#define CLIENT_H_

#include "Common.h"
#include "Config.h"
#include "Keyboard.h"
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

    virtual void SetXRadar(bool on) = 0;
    virtual void Warp() = 0;
    
    virtual int GetEnergy() = 0;
    virtual int GetRotation() = 0;
    virtual bool InSafe(Coord real_pos, const Level& level) = 0;

    virtual void Up(bool val) = 0;
    virtual void Down(bool val) = 0;
    virtual void Left(bool val) = 0;
    virtual void Right(bool val) = 0;

    virtual void Attach() = 0;

    virtual bool InShip() const = 0;
    virtual void EnterShip(int num) = 0;

    virtual std::vector<Coord> GetEnemies(Coord real_pos, const Level& level) = 0;
    virtual Coord GetClosestEnemy(Coord real_pos, const Level& level, int* dx, int* dy, double* dist) = 0;

    virtual Coord GetRealPosition(Coord bot_pos, Coord target, const Level& level) = 0;

    virtual void ReleaseKeys() = 0;
    virtual void ToggleKeys() = 0;
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

    Ships::RotationStore* m_Rotations;

    void GrabRadar();
public:
    ScreenClient(HWND hwnd, Config& config);

    virtual void Update(DWORD dt);

    virtual void Bomb();
    virtual void Gun(GunState state, int energy_percent = 100);

    virtual void SetXRadar(bool on);
    virtual void Warp();

    virtual int GetEnergy();
    virtual int GetRotation();
    virtual bool InSafe(Coord real_pos, const Level& level);

    virtual void Up(bool val);
    virtual void Down(bool val);
    virtual void Left(bool val);
    virtual void Right(bool val);

    virtual void Attach();

    virtual bool InShip() const;
    virtual void EnterShip(int num);

    virtual std::vector<Coord> GetEnemies(Coord real_pos, const Level& level);
    virtual Coord GetClosestEnemy(Coord real_pos, const Level& level, int* dx, int* dy, double* dist);

    virtual Coord GetRealPosition(Coord bot_pos, Coord target, const Level& level);

    virtual void ReleaseKeys();
    virtual void ToggleKeys();

    ScreenAreaPtr GetRadar() { return m_Radar; }
};

#endif
