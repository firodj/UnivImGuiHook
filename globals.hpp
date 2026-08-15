#pragma once

#include <windows.h>

namespace globals {
    extern HMODULE mainModule;
    extern HWND mainWindow;
    extern int uninjectKey;
    extern int openMenuKey;
    extern WNDPROC sOriginalWndProc;

    // Rendering backend currently in use
    enum class Backend {
        None,
        DX9,
        DX10,
        DX11,
        DX12,
        //Vulkan
    };
    extern Backend activeBackend;
    // Preferred backend to hook. None means auto with fallback order
    extern Backend preferredBackend;
    extern bool enableDebugLog;
    extern bool enableSimpleMenu;
    void SetDebugLogging(bool enable);

    void IncRefForDisableFileAccessLog();
    void DecRefForDisableFileAccessLog();
    bool HasRefForDisableFileAccessLog();

    void SetCustomInit(void (*pfnCustomInit)(void));    
    void CallCustomInit(void);
    void SetCustomRender(void (*pfnCustomRender)(void));
    void CallCustomRender(void);

    extern bool enableHookReadFile;
};

DWORD WINAPI ReinitializeGraphicalHooks(LPVOID lpParam);
