#include "Keyboard.h"
#include <thread>

INPUT Keyboard::GetInput(int keycode) {
    INPUT input;

    input.type = INPUT_KEYBOARD;
    input.ki.wScan = 0;
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;
    input.ki.dwFlags = 0;
    input.ki.wVk = keycode;

    return input;
}

void Keyboard::Send(int keycode) {
    INPUT input = GetInput(keycode);

    SendInput(1, &input, sizeof(INPUT));

    std::this_thread::sleep_for(std::chrono::milliseconds(15));

    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

void Keyboard::Up(int keycode) {
    INPUT input = GetInput(keycode);

    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
    m_Keys[keycode] = false;
}

void Keyboard::Down(int keycode) {
    INPUT input = GetInput(keycode);

    SendInput(1, &input, sizeof(INPUT));
    m_Keys[keycode] = true;
}
