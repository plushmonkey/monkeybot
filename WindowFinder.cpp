#include "WindowFinder.h"

BOOL CALLBACK WindowFinder::EnumProc(HWND hwnd, LPARAM lParam) {
    WindowFinder* self = reinterpret_cast<WindowFinder*>(lParam);

    TCHAR title_c[256];

    GetWindowText(hwnd, title_c, 256);

    tstring title(title_c, lstrlen(title_c));

    if (title.find(self->m_ToFind) != tstring::npos)
        self->m_Matches[hwnd] = title;

    return TRUE;
}