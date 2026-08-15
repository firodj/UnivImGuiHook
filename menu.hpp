#pragma once

#include <windows.h>

namespace menu {
    extern bool isOpen;
    void Init(HWND hwnd);
    void Render();
    void Hook();
    void Remove();
};
