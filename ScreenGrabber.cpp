#include "ScreenGrabber.h"
#include "ScreenArea.h"

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

bool ScreenGrabber::Save(const tstring& filename) {
    return m_Area->Save(filename);
}