#ifndef SCREENGRABBER_H_
#define SCREENGRABBER_H_

#include "Common.h"
#include <Windows.h>

class ScreenArea;

class ScreenGrabber {
private:
    HWND m_Window;
    int m_Width;
    int m_Height;
    int m_BPP;
    HDC m_ScreenDC;
    std::shared_ptr<ScreenArea> m_Area;

    void GetInfo(int* width, int* height, int* bpp);

public:
    ScreenGrabber(HWND hwnd);
    ~ScreenGrabber();

    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    const Pixel* GetPixels();
    Pixel GetPixel(int x, int y);
    void Update();
    std::shared_ptr<ScreenArea> GetArea(int x, int y, int width, int height);

    bool Save(const tstring& filename);
};

#endif
