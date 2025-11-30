#include "stdafx.h"
#include "globals.hpp"
#include "console.hpp"

namespace globals {
    // Handle to our DLL module
    HMODULE mainModule = nullptr;
    // Main game window handle
    HWND mainWindow = nullptr;
    // Key to uninject and exit (F11 by default)
    int uninjectKey = VK_F11;
    // Key to open/close the ImGui menu (INSERT by default)
    int openMenuKey = VK_INSERT;
    // Preferred backend to hook (None = auto fallback -> Not recommanded, specify your engine here)
    Backend preferredBackend = Backend::DX9;
    // Flag controlling runtime debug logging
    bool enableDebugLog = true;
    // Currently active rendering backend
    Backend activeBackend = Backend::None; // DO NOT MODIFY THIS LINE.

    WNDPROC sOriginalWndProc = nullptr;

    void SetDebugLogging(bool enable) {
        enableDebugLog = enable;
    }

    int refForDisableFALog = 0;

    void IncRefForDisableFileAccessLog() { refForDisableFALog++; }
    void DecRefForDisableFileAccessLog() { refForDisableFALog--; }
    bool HasRefForDisableFileAccessLog() { return refForDisableFALog > 0; }

    void (*g_pfnCustomInit)(void) = nullptr;
    void SetCustomInit(void (*pfnCustomInit)(void)) {
        g_pfnCustomInit = pfnCustomInit;
    }

    void CallCustomInit(void) {
        if (g_pfnCustomInit) {
            g_pfnCustomInit();
        }
    }

    void (*g_pfnCustomRender)(void) = nullptr;
    void SetCustomRender(void (*pfnCustomRender)(void)) {
        g_pfnCustomRender = pfnCustomRender;
    }

    void CallCustomRender(void) {
        if (g_pfnCustomRender) {
            g_pfnCustomRender();
        }
    }

    bool enableSimpleMenu = false;

    bool enableHookReadFile = false;
}

// Log initial global values for debugging
static void LogGlobals() {
    DebugLog("[Globals] mainModule=%p, mainWindow=%p, uninjectKey=0x%X, openMenuKey=0x%X, activeBackend=%d, preferredBackend=%d\n",
        globals::mainModule, globals::mainWindow, globals::uninjectKey, globals::openMenuKey,
        static_cast<int>(globals::activeBackend), static_cast<int>(globals::preferredBackend));
}

// Ensure we log when the DLL is loaded
struct GlobalsLogger {
    GlobalsLogger() {
        LogGlobals();
    }
};

static GlobalsLogger _globalsLogger;
