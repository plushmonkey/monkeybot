#ifndef SCREENAREA_H_
#define SCREENAREA_H_

#include "Common.h"

#include <Windows.h>

class ScreenArea {
public:
    typedef shared_ptr<ScreenArea> Ptr;

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
    Pixel GetPixel(int x, int y);
    ScreenArea::Ptr GetArea(int x, int y, int width, int height);
    bool Save(const tstring& filename);
    Vec2 Find(Pixel pixel);
};


#endif
