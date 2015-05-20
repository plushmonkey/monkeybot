#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <map>
#include <Windows.h>

class Keyboard {
private:
    std::map<int, bool> m_Keys;
    bool m_Toggled;
    INPUT GetInput(int keycode);

public:
    Keyboard();
    void Send(int keycode);
    void Up(int keycode);
    void Down(int keycode);
    void ToggleDown();
    void ReleaseAll();
};


#endif
