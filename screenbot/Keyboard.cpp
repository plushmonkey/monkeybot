#include "Keyboard.h"

#include <thread>

bool IsExtended(int keycode) {
    int extended[] = { VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, 
                       VK_HOME, VK_END, VK_PRIOR, VK_NEXT,
                       VK_INSERT, VK_DELETE };

    for (int i = 0; i < sizeof(extended) / sizeof(*extended); ++i) {
        if (keycode == extended[i])
            return true;
    }
    return false;
}

Keyboard::Keyboard() : m_Toggled(false) { }

INPUT Keyboard::GetInput(int keycode) {
    INPUT input;

    input.type = INPUT_KEYBOARD;
    input.ki.wScan = 0;
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;
    input.ki.dwFlags = 0;
    input.ki.wVk = keycode;

    if (IsExtended(keycode))
        input.ki.dwFlags = KEYEVENTF_EXTENDEDKEY;

    return input;
}

void Keyboard::Send(int keycode) {
    INPUT input = GetInput(keycode);

    SendInput(1, &input, sizeof(INPUT));

    std::this_thread::sleep_for(std::chrono::milliseconds(15));

    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));

    m_Keys[keycode] = false;
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

void Keyboard::ToggleDown() {
    for (auto& kv : m_Keys) {
        if (kv.second) {
            INPUT input = GetInput(kv.first);
            if (!m_Toggled)
                input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT));
        }
    }
    m_Toggled = !m_Toggled;
}

void Keyboard::ReleaseAll() {
    for (auto& kv : m_Keys) {
        if (kv.second) {
            INPUT input = GetInput(kv.first);
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT));
            kv.second = false;
        }
    }
}