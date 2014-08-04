#ifndef COMMON_H_
#define COMMON_H_

#include <cstdint>
#include <memory>
#include <ostream>

#ifdef UNICODE
#include <io.h>
#include <fcntl.h>
#define tstring std::wstring
#define tcout std::wcout
#define tcin std::wcin
#define tcerr std::wcerr
#define tostream std::wostream
#else
#define tstring std::string
#define tcout std::cout
#define tcin std::cin
#define tcerr std::cerr
#define tostream std::ostream
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

class ScreenArea;
class ScreenGrabber;

typedef std::shared_ptr<ScreenGrabber> ScreenGrabberPtr;
typedef std::shared_ptr<ScreenArea> ScreenAreaPtr;

namespace Ships {
    extern const u64 Rotations[8][40];
}

struct Pixel {
    u8 b;
    u8 g;
    u8 r;
    u8 a;

    Pixel() { }
    Pixel(u8 r, u8 g, u8 b, u8 a) : b(b), g(g), r(r), a(a) { }
};


tostream& operator<<(tostream& out, const Pixel& pix);
u64 operator+(const Pixel& pix, u64 a);
bool operator==(const Pixel& first, const Pixel& second);
bool operator!=(const Pixel& first, const Pixel& second);

struct Coord {
    int x;
    int y;

    Coord(int x, int y) : x(x), y(y) { }
};

tostream& operator<<(tostream& out, const Coord& coord);
bool operator==(const Coord& first, const Coord& second);
bool operator!=(const Coord& first, const Coord& second);

namespace Colors {
    extern const Pixel RadarColor;
    extern const Pixel RadarEnd;
    extern const Pixel RadarBorder;
    extern const Pixel SafeColor;
    extern const Pixel EnemyColor;
    extern const Pixel EnemyBallColor;
    extern const Pixel EnergyColor[2];
    extern const Pixel XRadarOff;
}

#endif
