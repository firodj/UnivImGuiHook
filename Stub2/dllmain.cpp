// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

// d3d9_proxy.cpp
#include <windows.h>
#include <iostream>
#include <vector>
#include <shellapi.h>

// Helper macro for debug logging via DebugView
inline void DebugLog(const char* fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    //std::cout << buf;
    OutputDebugStringA(buf);
}

static HMODULE real_d3d9 = nullptr;

// Declare pointers that will hold the addresses of real exports
extern "C" {
    // Add the exports you want to forward. These names must match the real export names.
    // Common d3d9 exports:
    void* ptr_Direct3DCreate9 = nullptr;
    void* ptr_D3DPERF_BeginEvent = nullptr;
    void* ptr_D3DPERF_EndEvent = nullptr;
    void* ptr_D3DPERF_SetMarker = nullptr;
    void* ptr_D3DPERF_SetRegion = nullptr;
    void* ptr_D3DPERF_QueryRepeatFrame = nullptr;
    void* ptr_D3DPERF_GetStatus = nullptr;
}

// Helper to load system d3d9.dll (from System32) to avoid loading local file again
static HMODULE LoadRealD3D9()
{
    CHAR sysPath[MAX_PATH];
    UINT n = GetSystemDirectoryA(sysPath, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return nullptr;
    strcat_s(sysPath, "\\d3d9.dll");
    DebugLog("[stub] Load real D3d9: %s\n", sysPath);
    return LoadLibraryA(sysPath);
}

std::vector<std::string> GetArgs()
{
    int argc = 0;

    // Get the full command line as UTF-16
    LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argvW) {
        return {}; // parsing failed
    }

    std::vector<std::string> args;
    args.reserve(argc);

    for (int i = 0; i < argc; ++i) {
        // Convert UTF-16 (Windows wide string) to UTF-8 std::string
        int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, nullptr, 0, nullptr, nullptr);
        std::string arg(sizeNeeded - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, arg.data(), sizeNeeded, nullptr, nullptr);
        args.push_back(std::move(arg));
    }

    LocalFree(argvW);

    return args;
}

// Creates a new thread to load your injected DLL (avoid heavy operations inside DllMain)
static DWORD WINAPI InjectorThread(LPVOID)
{
    bool expect_dll = false;
    std::string dll_path;
    auto args = GetArgs();
    
    for (size_t i = 1; i < args.size(); ++i) {
        auto arg = args[i];
        if (arg == "-dll") {
            expect_dll = true;
        }
        else if (expect_dll) {
            dll_path = arg;
        }
        else {
            std::cerr << "unknown arg: " << arg << std::endl;
            break;
        }
    }

    // Change "my_injected.dll" to your DLL name
    if (!dll_path.empty()) {
        DebugLog("[stub] inject dll %s\n", dll_path.c_str());
        
        HMODULE hModInject = LoadLibraryA(dll_path.c_str());
    }
    return 0;
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);

        real_d3d9 = LoadRealD3D9();
        if (!real_d3d9) {
            // failed to load system d3d9 Ereturn FALSE if you want to fail load
            return FALSE;
        }

        // Resolve the exported functions we will forward
        ptr_Direct3DCreate9 = (void*)GetProcAddress(real_d3d9, "Direct3DCreate9");
        ptr_D3DPERF_BeginEvent = (void*)GetProcAddress(real_d3d9, "D3DPERF_BeginEvent");
        ptr_D3DPERF_EndEvent = (void*)GetProcAddress(real_d3d9, "D3DPERF_EndEvent");
        ptr_D3DPERF_SetMarker = (void*)GetProcAddress(real_d3d9, "D3DPERF_SetMarker");
        ptr_D3DPERF_SetRegion = (void*)GetProcAddress(real_d3d9, "D3DPERF_SetRegion");
        ptr_D3DPERF_QueryRepeatFrame = (void*)GetProcAddress(real_d3d9, "D3DPERF_QueryRepeatFrame");
        ptr_D3DPERF_GetStatus = (void*)GetProcAddress(real_d3d9, "D3DPERF_GetStatus");

        // Launch injector on a new thread
        HANDLE h = CreateThread(nullptr, 0, InjectorThread, nullptr, 0, nullptr);
        if (h) CloseHandle(h);
      
    } else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        // Optionally free the real DLL
        if (real_d3d9) {
            FreeLibrary(real_d3d9);
            real_d3d9 = nullptr;
        }
     
    }
    return TRUE;
}

// -----------------------------
// Export stubs (x86 only)
// Each exported function is defined as a naked function that JMPs to the resolved address.
// The names below must match the exported names in the .def file (or the linker exports).
// -----------------------------

#ifdef _M_IX86

extern "C" {

    // Macro to declare a forwarder stub and associated pointer name
#define MAKE_FORWARD_STUB(name)               \
    __declspec(dllexport) void name();        \
    __declspec(naked) void name() {           \
        __asm {                               \
            jmp dword ptr [ptr_##name]        \
        }                                     \
    }

// Declare stubs
MAKE_FORWARD_STUB(Direct3DCreate9)
MAKE_FORWARD_STUB(D3DPERF_BeginEvent)
MAKE_FORWARD_STUB(D3DPERF_EndEvent)
MAKE_FORWARD_STUB(D3DPERF_SetMarker)
MAKE_FORWARD_STUB(D3DPERF_SetRegion)
MAKE_FORWARD_STUB(D3DPERF_QueryRepeatFrame)
MAKE_FORWARD_STUB(D3DPERF_GetStatus)

#undef MAKE_FORWARD_STUB

} // extern "C"

#else
// If not x86, compilation will fail here. See x64 notes below.
#error This source implements x86 (32-bit) forwarding. For x64 see comments in file.
#endif
