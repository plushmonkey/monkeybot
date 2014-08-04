#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <map>
#include <Windows.h>

class Keyboard {
private:
    std::map<int, bool> m_Keys;

    INPUT GetInput(int keycode);

public:
    void Send(int keycode);
    void Up(int keycode);
    void Down(int keycode);
};


#endif
