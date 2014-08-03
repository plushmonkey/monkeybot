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

struct Pixel {
    u8 b;
    u8 g;
    u8 r;
    u8 a;
};
typedef std::vector<Pixel> Pixels;

tostream& operator<<(tostream& out, const Pixel& pix) {
    out << "(" << (int)pix.r << ", " << (int)pix.g << ", " << (int)pix.b << ", " << (int)pix.a << ")";
    return out;
}

unsigned int operator+(const Pixel& pix, unsigned int a) {
    return ((pix.r << 16) | (pix.g << 8) | (pix.b)) + a;
}

bool operator==(const Pixel& first, const Pixel& second) {
    return first.r == second.r && first.g == second.g &&
           first.b == second.b && first.a == second.a;
}

namespace Keyboard {
    void Send(int keycode) {
        INPUT input;

        input.type = INPUT_KEYBOARD;
        input.ki.wScan = 0;
        input.ki.time = 0;
        input.ki.dwExtraInfo = 0;
        input.ki.dwFlags = 0;
        input.ki.wVk = keycode;

        SendInput(1, &input, sizeof(INPUT));

        input.ki.dwFlags = KEYEVENTF_KEYUP;
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        SendInput(1, &input, sizeof(INPUT));
    }

    void Down(int keycode) {
        INPUT input;

        input.type = INPUT_KEYBOARD;
        input.ki.wScan = 0;
        input.ki.time = 0;
        input.ki.dwExtraInfo = 0;
        input.ki.dwFlags = 0;
        input.ki.wVk = keycode;

        SendInput(1, &input, sizeof(INPUT));
    }

    void Up(int keycode) {
        INPUT input;

        input.type = INPUT_KEYBOARD;
        input.ki.wScan = 0;
        input.ki.time = 0;
        input.ki.dwExtraInfo = 0;
        input.ki.dwFlags = 0;
        input.ki.wVk = keycode;
        input.ki.dwFlags = KEYEVENTF_KEYUP;

        SendInput(1, &input, sizeof(INPUT));
    }
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

    if (!m_Bitmap) throw std::runtime_error("Could not create bitmap for ScreenArea.");
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

    if (m_BPP != 32)
        throw std::runtime_error("Color depth needs to be 32 bit.");
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

class Application {
private:
    WindowFinder m_Finder;

    HWND SelectWindow();
    int GetRotation(const ScreenArea::Ptr& area);

public:
    Application()
        : m_Finder(_T("Continuum"))
    { }

    int Run();
    void Update(ScreenGrabber& grabber, ScreenArea::Ptr& radar, ScreenArea::Ptr& ship);
};

HWND Application::SelectWindow() {
    const WindowFinder::Matches& matches = m_Finder.GetMatches();
    std::map<int, const WindowFinder::Match*> select_map;

    int i = 1;
    for (const WindowFinder::Match& kv : matches) {
        select_map[i] = &kv;
        tcout << i++ << " " << kv.second << std::endl;
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
    int val = 0;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++)
            val = area->GetPixel(16 + j, 16 + i) + val;
    }

    // default warbird graphics
    /*static const u64 warbird[] = { 125790036, 121778979, 119739135, 119930090, 117562575, 118485473, 117629391, 
                                   115525813, 115195824, 115201732, 114541490, 114605485, 116117951, 118679763, 
                                   120395014, 121775887, 118290407, 118351060, 116646096, 115326120, 116509113, 
                                   115586462, 114403993, 116647127, 115781020, 117495489, 117960928, 118289379, 
                                   120331790, 119933462, 123288362, 123159122, 128225150, 130333612, 129411739, 
                                   130133666, 130857387, 130859698, 127701410, 124672329 };*/
    // Hyperspace warbird graphics
    static const u64 warbird[] = { 113585296, 115626944, 114371716, 110756200, 108455251, 106942269, 107528738, 
                                   111006261, 109493802, 109166098, 105618426, 101613026, 99769543, 98122676, 
                                   99242694, 99569863, 97796777, 98915522, 97736655, 100498408, 102200807, 103706331, 
                                   104821966, 104622546, 104630241, 103517930, 102860749, 104902141, 105562397, 108456504,
                                   110557006, 111808621, 112466775, 111217761, 111350628, 110562670, 110230612, 111615099, 
                                   113193891, 113981331 };

    for (int i = 0; i < sizeof(warbird) / sizeof(*warbird); i++)
        if (warbird[i] == val) return i;
    
    return -1;
}

void Application::Update(ScreenGrabber& grabber, ScreenArea::Ptr& radar, ScreenArea::Ptr& ship) {
    grabber.Update();
    radar->Update();
    ship->Update();

    Pixel toFind;
    toFind.a = 255;
    toFind.r = 82;
    toFind.g = 82;
    toFind.b = 214;

    bool keydown = false;

    try {
        Coord pos = radar->Find(toFind);

        int dx = pos.x - 52;
        int dy = pos.y - 52;

        double angle = std::atan2(dy, dx) * 180 / M_PI;
        int target = static_cast<int>(angle / 9) + 10;
        if (target < 0) target += 40;

        double dist = std::sqrt(dx * dx + dy * dy);
        int rot = GetRotation(ship);

        if (rot != -1 && rot != target) {
            int away = std::abs(rot - target);
            int dir = 0;

            if (away < 20 && rot < target) dir = 1;
            if (away < 20 && rot > target) dir = 0;

            if (away > 20 && rot < target) dir = 0;
            if (away > 20 && rot > target) dir = 1;

            keydown = true;

            if (dir == 0) {
                Keyboard::Up(VK_RIGHT);
                Keyboard::Down(VK_LEFT);
            } else {
                Keyboard::Up(VK_LEFT);
                Keyboard::Down(VK_RIGHT);
            }

            if (dist > 15) {
                Keyboard::Up(VK_DOWN);
                Keyboard::Down(VK_UP);
            } else {
                Keyboard::Up(VK_UP);
                Keyboard::Down(VK_DOWN);
            }

            Keyboard::Down(VK_CONTROL);
        }

    } catch (const std::exception&) {
        if (keydown) {
            Keyboard::Up(VK_LEFT);
            Keyboard::Up(VK_RIGHT);
            Keyboard::Up(VK_UP);
            Keyboard::Up(VK_DOWN);
            Keyboard::Up(VK_CONTROL);
            keydown = false;
        }
    }
}

int Application::Run() {
    std::auto_ptr<ScreenGrabber> grabber;
    ScreenArea::Ptr ship, radar;
    HWND hwnd;
    
    try {
        hwnd = SelectWindow();

        grabber = std::auto_ptr<ScreenGrabber>(new ScreenGrabber(hwnd));
        ship = grabber->GetArea(grabber->GetWidth() / 2 - 18, grabber->GetHeight() / 2 - 18, 36, 36);
        radar = grabber->GetArea(530, 370, 104, 104);
    } catch (const std::exception& e) {
        tcerr << e.what() << std::endl;
        return 1;
    }

    while (true)
        Update(*grabber, radar, ship);

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
