#include "Bot.h"

#include "Common.h"
#include "Font.h"

#include <tchar.h>
#include <iostream>
#include <fstream>

#pragma comment(lib, "winmm.lib")

class Application {
private:
    Bot m_Bot;

public:
    Application() : m_Bot(8) { }

    int Run();
};

int Application::Run() {
    Fonts::TallFont = new Font(7, 10);

    if (!Fonts::TallFont->Load("fonts\\tallfont.bmp", 2)) {
        std::cerr << "Could not load fonts\\tallfont.bmp" << std::endl;
        std::abort();
    }

    return m_Bot.Run();
}

int wmain(int argc, wchar_t* argv[]) {
    Application app;

#ifdef UNICODE
    _setmode(_fileno(stdout), _O_U16TEXT);
#endif

    int rv = app.Run();
    return rv;
}
