#include "Common.h"
#include "Bot.h"

#pragma comment(lib, "winmm.lib")

class Application {
private:
    Bot m_Bot;

public:
    Application() : m_Bot(8) { }

    int Run();
};

int Application::Run() {
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
