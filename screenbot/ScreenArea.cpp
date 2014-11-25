#include "ScreenArea.h"

#include <fstream>

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
    return std::make_shared<ScreenArea>(m_DC, x, y, width, height);
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

Vec2 ScreenArea::Find(Pixel pixel) {
    for (int y = 0; y < m_Height; y++) {
        for (int x = 0; x < m_Width; x++) {
            if (GetPixel(x, y) == pixel)
                return Vec2(static_cast<float>(x), static_cast<float>(y));
        }
    }
    throw std::runtime_error("Could not find pixel.");
}