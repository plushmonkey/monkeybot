#define NOMINMAX
#include <iostream>
#include <windows.h>
#include <tchar.h>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <thread>
#define _USE_MATH_DEFINES
#include <math.h>
#include <limits>

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

namespace Ships {
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
}

struct Pixel {
    u8 b;
    u8 g;
    u8 r;
    u8 a;

    Pixel() { }
    Pixel(u8 r, u8 g, u8 b, u8 a) : b(b), g(g), r(r), a(a) { }
};

namespace Colors {
    const Pixel RadarColor(10, 25, 10, 0);
    const Pixel RadarEnd(132, 132, 132, 0);
    const Pixel SafeColor(24, 82, 24, 0);
    const Pixel EnemyColor(82, 82, 214, 0);
    const Pixel EnemyBallColor(255, 57, 74, 0);
    const Pixel EnergyColor[2] = { Pixel(181, 181, 255, 0), Pixel(99, 99, 206, 0) };
}


tostream& operator<<(tostream& out, const Pixel& pix) {
    out << "(" << (int)pix.r << ", " << (int)pix.g << ", " << (int)pix.b << ", " << (int)pix.a << ")";
    return out;
}

u64 operator+(const Pixel& pix, u64 a) {
    return ((pix.r << 16) | (pix.g << 8) | (pix.b)) + a;
}

bool operator==(const Pixel& first, const Pixel& second) {
    return first.r == second.r && first.g == second.g &&
           first.b == second.b;
}

bool operator!=(const Pixel& first, const Pixel& second) {
    return !(first == second);
}

struct Coord {
    int x;
    int y;

    Coord(int x, int y) : x(x), y(y) { }
};

tostream& operator<<(tostream& out, const Coord& coord) {
    out << "(" << coord.x << ", " << coord.y << ")";
    return out;
}

class ScreenArea {
public:
    typedef std::shared_ptr<ScreenArea> Ptr;

private:
    HDC m_Screen;
    HDC m_DC;
    int m_X;
    int m_Y;
    int m_Width;
    int m_Height;
    HBITMAP m_Bitmap;
    char *m_Data;

public:
    ScreenArea(HDC screendc, int x, int y, int width, int height);
    ~ScreenArea();

    int GetX() const { return m_X; }
    int GetY() const { return m_Y; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

    HDC GetDC() { return m_DC; }
    void Update();
    const Pixel* GetPixels();
    Pixel ScreenArea::GetPixel(int x, int y);
    ScreenArea::Ptr GetArea(int x, int y, int width, int height);
    bool Save(const tstring& filename);
    Coord Find(Pixel pixel);
};

ScreenArea::ScreenArea(HDC screendc, int x, int y, int width, int height)
    : m_Screen(screendc),
      m_DC(NULL), 
      m_X(x), 
      m_Y(y),
      m_Width(width),
      m_Height(height),
      m_Bitmap(NULL),
      m_Data(NULL)
{
    m_DC = CreateCompatibleDC(screendc);

    if (!m_DC) throw std::runtime_error("Could not create ScreenArea DC.");

    BITMAPINFO info;

    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = m_Width;
    info.bmiHeader.biHeight = m_Height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    m_Bitmap = CreateDIBSection(m_Screen, &info, DIB_RGB_COLORS, reinterpret_cast<void**>(&m_Data), 0, 0);
    SelectObject(m_DC, m_Bitmap);

    if (!m_Data || !m_Bitmap) throw std::runtime_error("Could not create bitmap for ScreenArea.");
}

ScreenArea::~ScreenArea() {
    if (m_Bitmap)
        DeleteObject(m_Bitmap);
    if (m_DC)
        DeleteDC(m_DC);
}

void ScreenArea::Update() {
    BitBlt(m_DC, 0, 0, m_Width, m_Height, m_Screen, m_X, m_Y, SRCCOPY);
}

const Pixel* ScreenArea::GetPixels() {
    return reinterpret_cast<const Pixel*>(m_Data);
}

Pixel ScreenArea::GetPixel(int x, int y) {
    if (x < 0 || x >= m_Width || y < 0 || y >= m_Height)
        throw std::runtime_error("GetPixel value out of range.");

    int ry = m_Height - y - 1;
    char* data = m_Data + ((m_Width * ry) + x) * 4;

    return *reinterpret_cast<Pixel *>(data);
}

ScreenArea::Ptr ScreenArea::GetArea(int x, int y, int width, int height) {
    ScreenArea::Ptr area(new ScreenArea(m_DC, x, y, width, height));
    return area;
}

bool ScreenArea::Save(const tstring& filename) {
    BITMAPFILEHEADER header;
    int offset = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    header.bfType = 'B' | ('M' << 8);
    header.bfOffBits = offset;
    header.bfSize = 0;
    header.bfReserved1 = header.bfReserved2 = 0;

    BITMAPINFOHEADER info = { 0 };

    info.biSize = sizeof(BITMAPINFOHEADER);
    info.biWidth = m_Width;
    info.biHeight = m_Height;
    info.biPlanes = 1;
    info.biBitCount = 32;
    info.biCompression = BI_RGB;

    std::fstream out(filename, std::ios::out | std::ios::binary);
    if (!out) return false;

    out.write((char *)&header, sizeof(BITMAPFILEHEADER));
    out.write((char *)&info, sizeof(BITMAPINFOHEADER));

    out.write(m_Data, m_Width * m_Height * 4);
    out.close();
    return true;
}

Coord ScreenArea::Find(Pixel pixel) {
    for (int y = 0; y < m_Height; y++) {
        for (int x = 0; x < m_Width; x++) {
            if (GetPixel(x, y) == pixel)
                return Coord(x, y);
        }
    }
    throw std::runtime_error("Could not find pixel.");
}

class ScreenGrabber {
private:
    HWND m_Window;
    int m_Width;
    int m_Height;
    int m_BPP;
    HDC m_ScreenDC;
    ScreenArea::Ptr m_Area;

    void GetInfo(int* width, int* height, int* bpp);

public:
    ScreenGrabber(HWND hwnd);
    ~ScreenGrabber();

    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    const Pixel* GetPixels();
    Pixel GetPixel(int x, int y);
    void Update();
    ScreenArea::Ptr GetArea(int x, int y, int width, int height);

    bool Save(const tstring& filename) {
        return m_Area->Save(filename);
    }
};

ScreenGrabber::ScreenGrabber(HWND hwnd)
    : m_Window(hwnd),
      m_Width(0),
      m_Height(0),
      m_BPP(0),
      m_ScreenDC(GetDC(hwnd))
{
    GetInfo(&m_Width, &m_Height, &m_BPP);

    m_Area = ScreenArea::Ptr(new ScreenArea(m_ScreenDC, 0, 0, m_Width, m_Height));
}

ScreenGrabber::~ScreenGrabber() {
    if (m_ScreenDC)
        ReleaseDC(m_Window, m_ScreenDC);
}

void ScreenGrabber::GetInfo(int* width, int* height, int* bpp) {
    HGDIOBJ hBitmap = GetCurrentObject(m_ScreenDC, OBJ_BITMAP);
    BITMAP bmp;

    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    RECT rect;
    GetClientRect(m_Window, &rect);
    *width = rect.right - rect.left;
    *height = rect.bottom - rect.top;
    *bpp = bmp.bmBitsPixel;
}

const Pixel* ScreenGrabber::GetPixels() {
    return m_Area->GetPixels();
}

Pixel ScreenGrabber::GetPixel(int x, int y) {
    return m_Area->GetPixel(x, y);
}

void ScreenGrabber::Update() {
    m_Area->Update();
}

ScreenArea::Ptr ScreenGrabber::GetArea(int x, int y, int width, int height) {
    return m_Area->GetArea(x, y, width, height);
}

/*
 * The WindowFinder object needs to stay in memory while EnumWindows runs or everything will mess up.
 */
class WindowFinder {
public:
    typedef std::pair<const HWND, tstring> Match;
    typedef std::map<HWND, tstring> Matches;

private:
    tstring m_ToFind;
    Matches m_Matches;

    static BOOL CALLBACK EnumProc(HWND hwnd, LPARAM lParam);
public:
    WindowFinder(const tstring &tofind)
        : m_ToFind(tofind) 
    {
        EnumWindows(EnumProc, reinterpret_cast<LPARAM>(this));
    }

    const Matches& GetMatches() const { return m_Matches; }
};

BOOL CALLBACK WindowFinder::EnumProc(HWND hwnd, LPARAM lParam) {
    WindowFinder* self = reinterpret_cast<WindowFinder*>(lParam);

    TCHAR title_c[256];

    GetWindowText(hwnd, title_c, 256);

    tstring title(title_c, lstrlen(title_c));

    if (title.find(self->m_ToFind) != tstring::npos)
        self->m_Matches[hwnd] = title;

    return TRUE;
}

class Keyboard {
private:
    std::map<int, bool> m_Keys;

    INPUT GetInput(int keycode);
public:
    void Send(int keycode);
    void Up(int keycode);
    void Down(int keycode);
};

INPUT Keyboard::GetInput(int keycode) {
    INPUT input;

    input.type = INPUT_KEYBOARD;
    input.ki.wScan = 0;
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;
    input.ki.dwFlags = 0;
    input.ki.wVk = keycode;

    return input;
}

void Keyboard::Send(int keycode) {
    INPUT input = GetInput(keycode);

    SendInput(1, &input, sizeof(INPUT));

    std::this_thread::sleep_for(std::chrono::milliseconds(15));

    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

void Keyboard::Up(int keycode) {
    INPUT input = GetInput(keycode);

    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
    m_Keys[keycode] = false;
}

void Keyboard::Down(int keycode) {
    INPUT input = GetInput(keycode);
        
    SendInput(1, &input, sizeof(INPUT));
    m_Keys[keycode] = true;
}

class Application {
private:
    WindowFinder m_Finder;
    HWND m_Window;
    Keyboard m_Keyboard;
    ScreenArea::Ptr m_EnergyArea[4];

    HWND SelectWindow();
    int GetRotation(const ScreenArea::Ptr& area);
    int GetEnergy(ScreenGrabber& grabber);
    int GetEnergyDigit(int digit);
    std::vector<Coord> GetEnemies(ScreenArea::Ptr& radar);

public:
    Application()
        : m_Finder(_T("Continuum")),
          m_Window(NULL)
    { }

    int Run();
    void Update(ScreenGrabber& grabber, ScreenArea::Ptr& radar, ScreenArea::Ptr& ship, ScreenArea::Ptr& player);
};

int Application::GetEnergyDigit(int digit) {
    Pixel toppix = m_EnergyArea[digit]->GetPixel(8, 1);
    Pixel toprightpix = m_EnergyArea[digit]->GetPixel(13, 4);
    Pixel topleftpix = m_EnergyArea[digit]->GetPixel(4, 4);
    Pixel middlepix = m_EnergyArea[digit]->GetPixel(7, 10);
    Pixel bottomleftpix = m_EnergyArea[digit]->GetPixel(1, 16);
    Pixel bottomrightpix = m_EnergyArea[digit]->GetPixel(10, 15);
    Pixel bottompix = m_EnergyArea[digit]->GetPixel(5, 19);

    bool top = toppix == Colors::EnergyColor[0] || toppix == Colors::EnergyColor[1];
    bool topright = toprightpix == Colors::EnergyColor[0] || toprightpix == Colors::EnergyColor[1];
    bool topleft = topleftpix == Colors::EnergyColor[0] || topleftpix == Colors::EnergyColor[1];
    bool middle = middlepix == Colors::EnergyColor[0] || middlepix == Colors::EnergyColor[1];
    bool bottomleft = bottomleftpix == Colors::EnergyColor[0] || bottomleftpix == Colors::EnergyColor[1];
    bool bottomright = bottomrightpix == Colors::EnergyColor[0] || bottomrightpix == Colors::EnergyColor[1];
    bool bottom = bottompix == Colors::EnergyColor[0] || bottompix == Colors::EnergyColor[1];

    if (top && topright && topleft && !middle && bottomleft && bottomright && bottom)
        return 0;

    if (!top && topright && !topleft && !middle && !bottomleft && bottomright && !bottom)
        return 1;

    if (top && topright && !topleft && middle && bottomleft && !bottomright && bottom)
        return 2;

    if (top && topright && !topleft && middle && !bottomleft && bottomright && bottom)
        return 3;

    if (!top && topright && topleft && middle && !bottomleft && bottomright && !bottom)
        return 4;

    if (top && !topright && topleft && middle && !bottomleft && bottomright && bottom)
        return 5;

    if (top && !topright && topleft && middle && bottomleft && bottomright && bottom)
        return 6;

    if (top && topright && !topleft && !middle && !bottomleft && bottomright && !bottom)
        return 7;

    if (top && topright && topleft && middle && bottomleft && bottomright && bottom)
        return 8;

    if (top && topright && topleft && middle && !bottomleft && bottomright && bottom)
        return 9;

    return 0;
}

int Application::GetEnergy(ScreenGrabber& grabber) {
    // Default to 2000 energy if resolution isn't supported
    if (!m_EnergyArea[0]) return 2000;

    for (int i = 0; i < 4; i++)
        m_EnergyArea[i]->Update();

    int first = GetEnergyDigit(0);
    int second = GetEnergyDigit(1);
    int third = GetEnergyDigit(2);
    int fourth = GetEnergyDigit(3);

    return (first * 1000) + (second * 100) + (third * 10) + fourth;
}

HWND Application::SelectWindow() {
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

    if (selection < 1 || selection >= i) {
        tcerr << "Error with window selection." << std::endl;
        return 0;
    }

    tcout << "Running bot on window " << select_map[selection]->second << "." << std::endl;

    return select_map[selection]->first;
}

int Application::GetRotation(const ScreenArea::Ptr& area) {
    u64 val = 0;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++)
            val = area->GetPixel(16 + j, 16 + i) + val;
    }

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 40; j++)
            if (Ships::Rotations[i][j] == val) return j;
    }
    
    return -1;
}

std::vector<Coord> Application::GetEnemies(ScreenArea::Ptr& radar) {
    std::vector<Coord> enemies;

    for (int y = 0; y < radar->GetWidth(); y++) {
        for (int x = 0; x < radar->GetWidth(); x++) {
            Pixel pix = radar->GetPixel(x, y);
            if (pix == Colors::EnemyColor || pix == Colors::EnemyBallColor) {
                int count = 0;
                try {
                    // up-left
                    Pixel pixel = radar->GetPixel(x - 1, y - 1);
                    if (pixel == Colors::EnemyColor || pixel == Colors::EnemyBallColor)
                        count++;
                    // up
                    pixel = radar->GetPixel(x, y - 1);
                    if (pixel == Colors::EnemyColor || pixel == Colors::EnemyBallColor)
                        count++;
                    // up-right
                    pixel = radar->GetPixel(x + 1, y - 1);
                    if (pixel == Colors::EnemyColor || pixel == Colors::EnemyBallColor)
                        count++;
                    // right
                    pixel = radar->GetPixel(x + 1, y);
                    if (pixel == Colors::EnemyColor || pixel == Colors::EnemyBallColor)
                        count++;
                    // left
                    pixel = radar->GetPixel(x - 1, y);
                    if (pixel == Colors::EnemyColor || pixel == Colors::EnemyBallColor)
                        count++;
                    // bottom-left
                    pixel = radar->GetPixel(x - 1, y + 1);
                    if (pixel == Colors::EnemyColor || pixel == Colors::EnemyBallColor)
                        count++;
                    // bottom-right
                    pixel = radar->GetPixel(x + 1, y + 1);
                    if (pixel == Colors::EnemyColor || pixel == Colors::EnemyBallColor)
                        count++;
                    // bottom
                    pixel = radar->GetPixel(x, y + 1);
                    if (pixel == Colors::EnemyColor || pixel == Colors::EnemyBallColor)
                        count++;
                } catch (std::exception&) {}

                if (count >= 3)
                    enemies.push_back(Coord(x, y));
            }
        }
    }

    if (enemies.size() == 0)
        throw std::runtime_error("No enemies near.");
    return enemies;
}

void Application::Update(ScreenGrabber& grabber, ScreenArea::Ptr& radar, ScreenArea::Ptr& ship, ScreenArea::Ptr& player) {
    grabber.Update();
    radar->Update();
    ship->Update();
    player->Update();

    bool insafe = false;

    try {
        player->Find(Colors::SafeColor);
        insafe = true;
    } catch (const std::exception&) {
        insafe = false;
    }

    int tardist = 10;

    static bool keydown = false;

    if (radar->GetWidth() > 300)
        tardist = 30;

    int energy = GetEnergy(grabber);

    try {
        std::vector<Coord> enemies = GetEnemies(radar);

        int radar_center = static_cast<int>(std::ceil(radar->GetWidth() / 2.0));

        int closest_dx = 0;
        int closest_dy = 0;
        double closest_dist = std::numeric_limits<double>::max();

        for (unsigned int i = 0; i < enemies.size(); i++) {
            int dx = enemies.at(i).x - radar_center;
            int dy = enemies.at(i).y - radar_center;

            double dist = std::sqrt(dx * dx + dy * dy);

            if (dist < closest_dist) {
                closest_dist = dist;
                closest_dx = dx;
                closest_dy = dy;
            }
        }

        double angle = std::atan2(closest_dy, closest_dx) * 180 / M_PI;
        int target = static_cast<int>(angle / 9) + 10;
        if (target < 0) target += 40;

        int rot = GetRotation(ship);

        keydown = true;

        if (rot != target) {
            int away = std::abs(rot - target);
            int dir = 0;

            if (away < 20 && rot < target) dir = 1;
            if (away < 20 && rot > target) dir = 0;

            if (away > 20 && rot < target) dir = 0;
            if (away > 20 && rot > target) dir = 1;

            if (dir == 0) {
                m_Keyboard.Up(VK_RIGHT);
                m_Keyboard.Down(VK_LEFT);
            } else {
                m_Keyboard.Up(VK_LEFT);
                m_Keyboard.Down(VK_RIGHT);
            }
        } else {
            m_Keyboard.Up(VK_LEFT);
            m_Keyboard.Up(VK_RIGHT);
        }
        
        if (energy < 600) {
            m_Keyboard.Down(VK_DOWN);
            m_Keyboard.Up(VK_UP);
            m_Keyboard.Up(VK_CONTROL);
        } else {
            if (closest_dist > tardist) {
                m_Keyboard.Up(VK_DOWN);
                m_Keyboard.Down(VK_UP);
            } else {
                m_Keyboard.Up(VK_UP);
                m_Keyboard.Down(VK_DOWN);
            }

            if (!insafe)
                m_Keyboard.Down(VK_CONTROL);
        }

        if (insafe)
            m_Keyboard.Up(VK_CONTROL);
        
    } catch (const std::exception&) {
        if (keydown) {
            m_Keyboard.Up(VK_LEFT);
            m_Keyboard.Up(VK_RIGHT);
            m_Keyboard.Up(VK_UP);
            m_Keyboard.Up(VK_DOWN);
            m_Keyboard.Up(VK_CONTROL);
            keydown = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int Application::Run() {
    std::auto_ptr<ScreenGrabber> grabber;
    ScreenArea::Ptr ship, radar, player;
    bool ready = false;

    while (!ready) {
        try {
            if (!m_Window)
                m_Window = SelectWindow();

            grabber = std::auto_ptr<ScreenGrabber>(new ScreenGrabber(m_Window));
            ship = grabber->GetArea(grabber->GetWidth() / 2 - 18, grabber->GetHeight() / 2 - 18, 36, 36);

            int radarstart = 0;
            int radarend = 0;
            int radary = 0;

            while (radarend == 0) {
                grabber->Update();

                for (int y = grabber->GetHeight() / 2; y < grabber->GetHeight(); y++) {
                    for (int x = grabber->GetWidth() / 2; x < grabber->GetWidth(); x++) {
                        Pixel pix = grabber->GetPixel(x, y);

                        if (radarstart == 0 && pix == Colors::RadarColor)
                            radarstart = x;

                        if (radarstart != 0 && pix == Colors::RadarEnd) {
                            radarend = x;
                            radary = y;
                            x = grabber->GetWidth();
                            y = grabber->GetHeight();
                            break;
                        }
                    }
                }

                std::this_thread::sleep_for(std::chrono::seconds(1));
                tcout << "Finding radar location." << std::endl;
            }
            
            int radarwidth = radarend - radarstart;

            tcout << "Creating radar screen with width of " << radarwidth << " at " << radarstart << ", " << radary << "." << std::endl;

            radar = grabber->GetArea(radarstart, radary, radarwidth, radarwidth);

            if (!radar.get()) {
                tcerr << "Resolution (" << grabber->GetWidth() << "x" << grabber->GetHeight() << ") not supported." << std::endl;
                std::abort();
            }

            player = radar->GetArea(radar->GetWidth() / 2 - 1, radar->GetWidth() / 2 - 1, 4, 4);

            if (grabber->GetWidth() == 640) {
                m_EnergyArea[0] = grabber->GetArea(562, 0, 16, 21);
                m_EnergyArea[1] = grabber->GetArea(578, 0, 16, 21);
                m_EnergyArea[2] = grabber->GetArea(594, 0, 16, 21);
                m_EnergyArea[3] = grabber->GetArea(610, 0, 16, 21);
            }

            ready = true;
        } catch (const std::exception& e) {
            tcerr << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }

    while (true)
        Update(*grabber, radar, ship, player);

    return 0;
}

int wmain(int argc, wchar_t* argv[]) {
    Application app;

#ifdef UNICODE
    _setmode(_fileno(stdout), _O_U16TEXT);
#endif

    int rv = app.Run();
    return rv;
}
