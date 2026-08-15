#include "kernel32hook.hpp"

// dllmain.cpp forward declaration
void InitForModule(const char* name);

namespace kernel32hook {
    LoadLibraryA_t oLoadLibraryA = nullptr;
    LoadLibraryW_t oLoadLibraryW = nullptr;

    // Hooked LoadLibraryA
    HMODULE WINAPI hookLoadLibraryA(LPCSTR lpLibFileName)
    {
        HMODULE mod = oLoadLibraryA(lpLibFileName);
        if (mod)
            InitForModule(lpLibFileName);
        return mod;
    }

    // Hooked LoadLibraryW
    HMODULE WINAPI hookLoadLibraryW(LPCWSTR lpLibFileName)
    {
        HMODULE mod = oLoadLibraryW(lpLibFileName);
        if (mod && lpLibFileName)
        {
            char name[MAX_PATH];
            WideCharToMultiByte(CP_ACP, 0, lpLibFileName, -1, name, MAX_PATH, nullptr, nullptr);
            InitForModule(name);
        }
        return mod;
    }
}
