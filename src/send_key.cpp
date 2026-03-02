#include <windows.h>
#include <iostream>

// Send a single keypress using Windows SendInput API
void SendKeyPress(WORD vkCode)
{
    INPUT inputs[2] = {};

    // Key down
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vkCode;
    inputs[0].ki.dwFlags = 0;

    // Key up
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = vkCode;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    UINT result = SendInput(2, inputs, sizeof(INPUT));
    if (result != 2)
    {
        std::cerr << "SendInput failed: " << GetLastError() << std::endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: send_key <key>" << std::endl;
        std::cerr << "Example: send_key z" << std::endl;
        return 1;
    }

    char key = argv[1][0];
    WORD vkCode = 0;

    // Map common keys to virtual key codes
    if (key == 'z' || key == 'Z')
    {
        vkCode = 'Z';
    }
    else if (key == 'x' || key == 'X')
    {
        vkCode = 'X';
    }
    else if (key == ' ')
    {
        vkCode = VK_SPACE;
    }
    else
    {
        // Try to use the character directly (works for A-Z)
        vkCode = VkKeyScan(key) & 0xFF;
    }

    if (vkCode == 0)
    {
        std::cerr << "Unsupported key: " << key << std::endl;
        return 1;
    }

    SendKeyPress(vkCode);
    return 0;
}
