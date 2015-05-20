#ifndef FONT_H_
#define FONT_H_

#include "Common.h"

#include <string>
#include <map>
#include <vector>

#define MAX_ROWS 20
#define MAX_COLS 20

class Character {
private:
    int m_Width;
    int m_Height;
    int m_RowValues[MAX_ROWS];
    int m_ColValues[MAX_COLS];
    char m_Char;
public:
    Character(int width, int height) 
        : m_Width(width), 
          m_Height(height), 
          m_Char(' ')
    { 
        memset(m_RowValues, 0, sizeof(int)*MAX_ROWS);
        memset(m_ColValues, 0, sizeof(int)*MAX_COLS);
    }

    ~Character() { }

    Character(const Character& other) {
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        m_Char = other.m_Char;

        memcpy(m_RowValues, other.m_RowValues, sizeof(int) * m_Height);
        memcpy(m_ColValues, other.m_ColValues, sizeof(int) * m_Width);
    }

    void SetRowCount(int row, int amount) {
        m_RowValues[row] = amount;
    }

    void SetColCount(int col, int amount) {
        m_ColValues[col] = amount;
    }

    void SetChar(char c) {
        m_Char = c;
    }

    bool operator==(const Character& other) const {
        for (int i = 0; i < m_Width; ++i) {
            if (m_ColValues[i] != other.m_ColValues[i])
                return false;
        }

        for (int i = 0; i < m_Height; ++i) {
            if (m_RowValues[i] != other.m_RowValues[i])
                return false;
        }

        return true;
    }

    bool operator<(const Character& other) const {
        return m_Char < other.m_Char;
    }
};

class Font {
private:
    std::map<Character, char> m_Characters;
    int m_CharWidth;
    int m_CharHeight;

public:
    Font(int char_width, int char_height);

    bool Load(const std::string& filename, int row_start);

    Character GetCharacter(char c);
    char GetCharacter(Character c);
    bool GetCharacter(ScreenAreaPtr area, const std::vector<Pixel>& ignore, char* out);

    int GetWidth() const { return m_CharWidth; }
    int GetHeight() const { return m_CharHeight; }
};

#endif
