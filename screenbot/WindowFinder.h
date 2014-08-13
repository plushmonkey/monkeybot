#ifndef WINDOWFINDER_H_
#define WINDOWFINDER_H_

#include "Common.h"
#include <Windows.h>
#include <map>

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

#endif
