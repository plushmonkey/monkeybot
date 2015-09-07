#ifndef CLIENT_H_
#define CLIENT_H_

#include "Common.h"
#include "Config.h"
#include "Keyboard.h"
#include "PlayerList.h"
#include "api/Client.h"

#include <Windows.h>
#include <vector>
#include <unordered_map>
#include <mutex>

class Level;

namespace Ships {
    class RotationStore;
}

namespace Memory {
    class MemorySensor;
}

class ScreenClient : public api::Client {
private:
    Config& m_Config;
    Keyboard m_Keyboard;
    HWND m_Window;

    ScreenGrabberPtr m_Screen;
    ScreenAreaPtr m_Radar;
    ScreenAreaPtr m_Ship;
    ScreenAreaPtr m_Player;
    ScreenAreaPtr m_EnergyArea[5];
    PlayerWindow m_PlayerWindow;

    typedef std::function<void(ScreenGrabberPtr, int, int)> PixelHandler;
    std::unordered_map<Pixel, PixelHandler> m_PixelHandlers;

    std::string m_Target;
    std::string m_PriorityTarget;

    Memory::MemorySensor& m_MemorySensor;

    int m_CurrentBulletDelay;

    DWORD m_LastBullet;
    DWORD m_LastBomb;
    DWORD m_EmpEnd;
    std::mutex m_ChatMutex;

    bool m_Multi;
    MultiState m_MultiState;

    bool m_Thrusting;

    Ships::RotationStore* m_Rotations;

    MultiState DetermineMultiState() const;
    void GrabRadar();
public:
    ScreenClient(HWND hwnd, Config& config, Memory::MemorySensor& memsensor);

    virtual void Update(DWORD dt);

    virtual void Bomb();
    virtual void Gun(GunState state, int energy_percent = 100);
    virtual void Burst();
    virtual void Repel();
	virtual void Decoy();
    virtual void SetThrust(bool on);

    virtual void SetXRadar(bool on);
    virtual void Warp();

    virtual int GetEnergy(api::Ship current_ship);
    virtual int GetRotation();
    virtual bool IsInSafe(Vec2 real_pos, const Level& level) const;

    virtual void Up(bool val);
    virtual void Down(bool val);
    virtual void Left(bool val);
    virtual void Right(bool val);

    virtual void Attach();

    virtual bool InShip() const;
    virtual void EnterShip(int num);
    virtual void Spec();

    virtual std::vector<api::PlayerPtr> GetEnemies();
    virtual api::PlayerPtr GetClosestEnemy(Vec2 real_pos, Vec2 heading, const Level& level, int* dx, int* dy, double* dist);

    virtual Vec2 GetRealPosition(Vec2 bot_pos, Vec2 target, const Level& level);

    virtual void ReleaseKeys();
    virtual void ToggleKeys();

    virtual int GetFreq();

    virtual api::PlayerList GetFreqPlayers(int freq);
    virtual api::PlayerList GetPlayers();

    virtual bool OnSoloFreq();
    virtual api::PlayerPtr GetSelectedPlayer();
    virtual void MoveTicker(Direction dir);

    ScreenAreaPtr GetRadar() const { return m_Radar; }

    virtual std::vector<Vec2> FindMines(Vec2 bot_pixel_pos);

    void EMPPixelHandler(ScreenGrabberPtr screen, int x, int y);
    virtual void Scan();

    virtual bool Emped();

    virtual void SendString(const std::string& str, bool paste = true);
    virtual void SendPM(const std::string& target, const std::string& mesg);
    virtual void UseMacro(short num);

    virtual void SelectPlayer(const std::string& name);

    virtual void SetTarget(const std::string& name);
    virtual void SetPriorityTarget(const std::string& name);

    virtual std::string GetTarget() const {
        return m_Target;
    }
    virtual std::string GetPriorityTarget() const {
        return m_PriorityTarget;
    }
    virtual void EnableMulti(bool enable);
    virtual MultiState GetMultiState() const {
        return m_MultiState;
    }
    void Rocket();
    Vec2 GetResolution();
    bool HasBouncingBullets() const;
};

#endif
