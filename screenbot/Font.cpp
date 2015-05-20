#include "Font.h"

#include "Common.h"
#include "ScreenArea.h"

#include <Windows.h>
#include <memory>
#include <iostream>

Font::Font(int char_width, int char_height)
    : m_CharWidth(char_width),
      m_CharHeight(char_height)
{

}

bool Font::Load(const std::string& filename, int row_start) {
    HBITMAP hBMP = (HBITMAP) LoadImage(NULL, filename.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

    if (!hBMP) return false;

    HDC dc = CreateCompatibleDC(NULL);
    BITMAP bmp;

    GetObject(hBMP, sizeof(BITMAP), &bmp);
    SelectObject(dc, hBMP);

    BITMAPINFO bmpInfo;
    bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfo.bmiHeader.biWidth = bmp.bmWidth;
    bmpInfo.bmiHeader.biHeight = -bmp.bmHeight;
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biBitCount = 32;
    bmpInfo.bmiHeader.biCompression = BI_RGB;
    bmpInfo.bmiHeader.biSizeImage = 0;

    COLORREF* pixels = new COLORREF[bmp.bmWidth * bmp.bmHeight];
    GetDIBits(dc, hBMP, 0, bmp.bmHeight, pixels, &bmpInfo, DIB_RGB_COLORS);

    auto GetPixel = [&](int x, int y) -> Pixel {
        DWORD val = pixels[y * bmp.bmWidth + x];
        Pixel pix;
        pix.a = 0;
        pix.r = static_cast<u8>((val & 0xFF0000) >> 16);
        pix.g = (val & 0x00FF00) >> 8;
        pix.b = (val & 0x0000FF);
        return pix;
    };

    const int CharPerRow = bmp.bmWidth / (m_CharWidth + 1);
    const int TopPadding = 1;
    const int RowPadding = 2;
    const Pixel BGPixel(0, 0, 0, 0);
    char current_char = ' ';

    for (int cur_row = row_start; cur_row < row_start + 2; ++cur_row) {
        for (int cur_char = 0; cur_char < CharPerRow; ++cur_char) {
            Pixel* char_pixels = new Pixel[m_CharHeight * m_CharWidth];
            Character c(m_CharWidth, m_CharHeight);
            
            std::unique_ptr<int[]> col_count(new int[m_CharWidth]);

            for (int i = 0; i < m_CharWidth; ++i)
                col_count[i] = 0;

            for (int y = 0; y < m_CharHeight; ++y) {
                int row_count = 0;
                for (int x = 0; x < m_CharWidth; ++x) {
                    int cur_x = (cur_char * m_CharWidth) + cur_char + x;
                    int cur_y = TopPadding + (cur_row * RowPadding) + (cur_row * m_CharHeight) + y;

                    if (GetPixel(cur_x, cur_y) != BGPixel) {
                        row_count++;
                        col_count[x]++;
                    }
                }
                c.SetRowCount(y, row_count);
            }

            for (int i = 0; i < m_CharWidth; ++i)
                c.SetColCount(i, col_count[i]);

            c.SetChar(current_char);
            m_Characters[c] = current_char++;

            delete[] char_pixels;
        }
    }

    DeleteDC(dc);
    delete[] pixels;
    return hBMP != NULL;
}

Character Font::GetCharacter(char c) {
    for (auto& kv : m_Characters) {
        if (kv.second == c)
            return kv.first;
    }
    return Character(m_CharWidth, m_CharHeight);
}

char Font::GetCharacter(Character c) {
    return m_Characters[c];
}

bool Font::GetCharacter(ScreenAreaPtr area, const std::vector<Pixel>& ignore, char* out) {
    Character check_char(m_CharWidth, m_CharHeight);

    std::unique_ptr<int[]> col_count(new int[m_CharWidth]);

    for (int i = 0; i < m_CharWidth; ++i)
        col_count[i] = 0;

    for (int y = 0; y < m_CharHeight; ++y) {
        int row_count = 0;
        for (int x = 0; x < m_CharWidth; ++x) {
            Pixel pix = area->GetPixel(x, y);

            bool add = true;
            for (const Pixel& ignpix : ignore) {
                if (pix == ignpix) {
                    add = false;
                    break;
                }
            }

            if (add) {
                row_count++;
                col_count[x]++;
            }
        }
        check_char.SetRowCount(y, row_count);
        for (int i = 0; i < m_CharWidth; ++i)
            check_char.SetColCount(i, col_count[i]);
    }
   
    for (auto& kv : m_Characters) {
        if (kv.first == check_char) {
            *out = kv.second;
            return true;
        }
    }

    return false;
}
