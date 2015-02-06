#ifndef CLIENT_SETTINGS_H_
#define CLIENT_SETTINGS_H_

#include <cstdint>

struct ShipSettings {
    uint32_t SuperTime;                 // 0x04
    uint32_t ShieldsTime;               // 0x08
    int16_t  Gravity;                   // 0x0C
    int16_t  GravityTopSpeed;           // 0x0E
    uint16_t BulletFireEnergy;          // 0x10
    uint16_t MultiFireEnergy;           // 0x12
    uint16_t BombFireEnergy;            // 0x14
    uint16_t BombFireEnergyUpgrade;     // 0x16
    uint16_t LandmineFireEnergy;        // 0x18
    uint16_t LandmineFireEnergyUpgrade; // 0x1A
    uint16_t BulletSpeed;               // 0x1C
    uint16_t BombSpeed;                 // 0x1E
    struct {
        uint16_t SeeBombLevel : 2;
        uint16_t DisableFastShooting : 1;
        uint16_t Radius : 8;
        uint16_t padding : 5;
    };                                  // 0x20
    uint16_t MultiFireAngle;            // 0x22
    uint16_t CloakEnergy;               // 0x24
    uint16_t StealthEnergy;             // 0x26
    uint16_t AntiWarpEnergy;            // 0x28
    uint16_t XRadarEnergy;              // 0x2A
    uint16_t MaximumRotation;           // 0x2C
    uint16_t MaximumThrust;             // 0x2E
    uint16_t MaximumSpeed;              // 0x30
    uint16_t MaximumRecharge;           // 0x32
    uint16_t MaximumEnergy;             // 0x34
    uint16_t InitialRotation;           // 0x36
    uint16_t InitialThrust;             // 0x38
    uint16_t InitialSpeed;              // 0x3A
    uint16_t InitialRecharge;           // 0x3C
    uint16_t InitialEnergy;             // 0x3E
    uint16_t UpgradeRotation;           // 0x40
    uint16_t UpgradeThrust;             // 0x42
    uint16_t UpgradeSpeed;              // 0x44
    uint16_t UpgradeRecharge;           // 0x46
    uint16_t UpgradeEnergy;             // 0x48
    uint16_t AfterburnerEnergy;         // 0x4A
    uint16_t BombThrust;                // 0x4C
    uint16_t BurstSpeed;                // 0x4E
    uint16_t TurretThrustPenalty;       // 0x50
    uint16_t TurretSpeedPenalty;        // 0x52
    uint16_t BulletFireDelay;           // 0x54
    uint16_t MultiFireDelay;            // 0x56
    uint16_t BombFireDelay;             // 0x58
    uint16_t LandmineFireDelay;         // 0x5A
    uint16_t RocketTime;                // 0x5C
    uint16_t InitialBounty;             // 0x5E
    uint16_t DamageFactor;              // 0x60
    uint16_t PrizeShareLimit;           // 0x62
    uint16_t AttachBounty;              // 0x64
    uint16_t SoccerBallThrowTimer;      // 0x66
    uint16_t SoccerBallFriction;        // 0x68
    uint16_t SoccerBallProximity;       // 0x6A
    uint16_t SoccerBallSpeed;           // 0x6C
    uint8_t  TurretLimit;               // 0x6E
    uint8_t  BurstShrapnel;             // 0x6F
    uint8_t  MaxMines;                  // 0x70
    uint8_t  RepelMax;                  // 0x71
    uint8_t  BurstMax;                  // 0x72
    uint8_t  DecoyMax;                  // 0x73
    uint8_t  BrickMax;                  // 0x74
    uint8_t  ThorMax;                   // 0x75
    uint8_t  RocketMax;                 // 0x76
    uint8_t  PortalMax;                 // 0x77
    uint8_t  InitialRepel;              // 0x78
    uint8_t  InitialBurst;              // 0x79
    uint8_t  InitialBrick;              // 0x7A
    uint8_t  InitialRocket;             // 0x7B
    uint8_t  InitialThor;               // 0x7C
    uint8_t  InitialDecoy;              // 0x7D
    uint8_t  InitialPortal;             // 0x7E
    uint8_t  BombBounceCount;           // 0x7F
    struct {
        uint8_t ShrapnelMax : 5;
        uint8_t ShrapnelRate : 3;
    };                                  // 0x80
    struct {
        uint8_t UnknownStatus : 2;
        uint8_t CloakStatus : 2;
        uint8_t StealthStatus : 2;
        uint8_t XRadarStatus : 2;
    };                                  // 0x81
    struct {
        uint8_t AntiWarpStatus : 2;
        uint8_t InitialGuns : 2;
        uint8_t MaxGuns : 2;
        uint8_t InitialBombs : 2;
    };                                  // 0x82
    struct {
        uint8_t MaxBombs : 2;
        uint8_t DoubleBarrel : 1;
        uint8_t EmpBomb : 1;
        uint8_t SeeMines : 1;
        uint8_t padding2 : 3;
    };                                  // 0x83
};

#endif
