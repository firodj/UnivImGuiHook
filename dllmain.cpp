#include "stdafx.h"

#include <mutex>
#include <thread>

#include "console.hpp"
#include "globals.hpp"
#include "menu.hpp"
#include "util.hpp"
#include "kernel32/kernel32hook.hpp"
#include "di8wrap/di_wrap.hpp"

namespace mousehooks { void Init(); void Remove(); }

// Utility helpers for backend initialization checks
using IsInitFn = bool (*)();

static bool WaitForInitialization(IsInitFn fn, int attempts = 50, int sleepMs = 100)
{
    for (int i = 0; i < attempts; ++i)
    {
        if (fn())
            return true;
        Sleep(sleepMs);
    }
    return false;
}

static bool TryInitBackend(globals::Backend backend)
{
    switch (backend)
    {
#ifdef ENABLE_BACKEND_VULKAN
    case globals::Backend::Vulkan:
        if (GetModuleHandleA("vulkan-1.dll"))
        {
            DebugLog("[DllMain] Attempting Vulkan initialization.\n");
            hooks_vk::Init();
            if (WaitForInitialization(hooks_vk::IsInitialized))
            {
                DebugLog("[DllMain] Vulkan initialization succeeded.\n");
                globals::activeBackend = globals::Backend::Vulkan;
                return true;
            }
            DebugLog("[DllMain] Vulkan initialization pending, hooks remain active.\n");
        }
        break;
#endif
#ifdef ENABLE_BACKEND_DX12
    case globals::Backend::DX12:
        if (GetModuleHandleA("d3d12.dll") || GetModuleHandleA("dxgi.dll"))
        {
            DebugLog("[DllMain] Attempting DX12 initialization.\n");
            d3d12hook::Init();
            if (WaitForInitialization(d3d12hook::IsInitialized))
            {
                DebugLog("[DllMain] DX12 initialization succeeded.\n");
                globals::activeBackend = globals::Backend::DX12;
                return true;
            }
            DebugLog("[DllMain] DX12 initialization failed, falling back.\n");
            d3d12hook::release();
        }
        break;
#endif
#ifdef ENABLE_BACKEND_DX11
    case globals::Backend::DX11:
        if (GetModuleHandleA("d3d11.dll"))
        {
            DebugLog("[DllMain] Attempting DX11 initialization.\n");
            hooks_dx11::Init();
            if (WaitForInitialization(hooks_dx11::IsInitialized))
            {
                DebugLog("[DllMain] DX11 initialization succeeded.\n");
                globals::activeBackend = globals::Backend::DX11;
                return true;
            }
            DebugLog("[DllMain] DX11 initialization failed, falling back.\n");
            hooks_dx11::release();
        }
        break;
#endif
#ifdef ENABLE_BACKEND_DX10
    case globals::Backend::DX10:
        if (GetModuleHandleA("d3d10.dll"))
        {
            DebugLog("[DllMain] Attempting DX10 initialization.\n");
            hooks_dx10::Init();
            if (WaitForInitialization(hooks_dx10::IsInitialized))
            {
                DebugLog("[DllMain] DX10 initialization succeeded.\n");
                globals::activeBackend = globals::Backend::DX10;
                return true;
            }
            DebugLog("[DllMain] DX10 initialization failed, falling back.\n");
            hooks_dx10::release();
        }
        break;
#endif
#ifdef ENABLE_BACKEND_DX9
    case globals::Backend::DX9:
        if (GetModuleHandleA("d3d9.dll"))
        {
            DebugLog("[DllMain] Attempting DX9 initialization.\n");
            d3d9hook::Init();
#if 0
            // SKIP - initialization early, just wait when dx9 being called
            if (WaitForInitialization(d3d9hook::IsInitialized))
            {
                DebugLog("[DllMain] DX9 initialization succeeded.\n");
                globals::activeBackend = globals::Backend::DX9;
                return true;
            }
            DebugLog("[DllMain] DX9 initialization failed, falling back.\n");
            d3d9hook::release();
#endif
        }
        break;
#endif
    default:
        break;
    }
    return false;
}

static bool TryInitializeFrom(globals::Backend start)
{
    const globals::Backend order[] = {
#ifdef ENABLE_BACKEND_VULKAN
        globals::Backend::Vulkan,
#endif
#ifdef ENABLE_BACKEND_DX12
        globals::Backend::DX12,
#endif
#ifdef ENABLE_BACKEND_DX11
        globals::Backend::DX11,
#endif
#ifdef ENABLE_BACKEND_DX10
        globals::Backend::DX10,
#endif
#ifdef ENABLE_BACKEND_DX9
        globals::Backend::DX9
#endif
    };
    int idx = 0;
    for (; idx < 5; ++idx)
    {
        if (order[idx] == start)
            break;
    }
    for (; idx < 5; ++idx)
    {
        if (TryInitBackend(order[idx]))
            return true;
    }
    DebugLog("[DllMain] All backend initialization attempts failed.\n");
    return false;
}

// Helper: check loaded module name and initialize hooks if needed
static int GetBackendPriority(globals::Backend backend)
{
    switch (backend)
    {
#ifdef ENABLE_BACKEND_VULKAN
    case globals::Backend::Vulkan: return 5;
#endif
#ifdef ENABLE_BACKEND_DX12
    case globals::Backend::DX12:   return 4;
#endif
#ifdef ENABLE_BACKEND_DX11
    case globals::Backend::DX11:   return 3;
#endif
#ifdef ENABLE_BACKEND_DX10
    case globals::Backend::DX10:   return 2;
#endif
#ifdef ENABLE_BACKEND_DX9
    case globals::Backend::DX9:    return 1;
#endif
    default:                       return 0;
    }
}

void InitForModule(const char* name)
{
    if (!name)
        return;

	const char* base = strrchr(name, '\\');
    base = base ? base + 1 : name;

    globals::Backend detected = globals::Backend::None;
    // if (_stricmp(base, "vulkan-1.dll") == 0) {
    //    detected = globals::Backend::Vulkan;
    //}
    if (_stricmp(base, "d3d12.dll") == 0 || _stricmp(base, "dxgi.dll") == 0) {
        detected = globals::Backend::DX12;
    }
    else if (_stricmp(base, "d3d11.dll") == 0) {
        detected = globals::Backend::DX11;
    }
    else if (_stricmp(base, "d3d10.dll") == 0) {
        detected = globals::Backend::DX10;
    }
    else if (_stricmp(base, "d3d9.dll") == 0) {
        detected = globals::Backend::DX9;
    }
    else {
        return;
    }

    DebugLog("[DllMain] InitForModule: %s\n", name);
    if (globals::preferredBackend != globals::Backend::None && detected != globals::preferredBackend)
        return;

    if (GetBackendPriority(detected) <= GetBackendPriority(globals::activeBackend))
        return;

    switch (globals::activeBackend)
    {
#ifdef ENABLE_BACKEND_DX9
    case globals::Backend::DX9:    d3d9hook::release(); break;
#endif
#ifdef ENABLE_BACKEND_DX10
    case globals::Backend::DX10:   hooks_dx10::release(); break;
#endif
#ifdef ENABLE_BACKEND_DX11
    case globals::Backend::DX11:   hooks_dx11::release(); break;
#endif
#ifdef ENABLE_BACKEND_DX12
    case globals::Backend::DX12:   d3d12hook::release(); break;
#endif
#ifdef ENABLE_BACKEND_VULKAN
    case globals::Backend::Vulkan: hooks_vk::release(); break;
#endif
    default: break;
    }

    globals::activeBackend = globals::Backend::None;
    if (globals::preferredBackend != globals::Backend::None)
        TryInitBackend(globals::preferredBackend);
    else
        TryInitializeFrom(detected);
}

// Thread routine that performs cleanup and unloads the DLL
static DWORD WINAPI UninjectThread(LPVOID)
{
    DebugLog("[DllMain] Uninject thread starting.\n");

    switch (globals::activeBackend)
    {
#ifdef ENABLE_BACKEND_DX9
    case globals::Backend::DX9:
        d3d9hook::release();
        break;
#endif
#ifdef ENABLE_BACKEND_DX10
    case globals::Backend::DX10:
        hooks_dx10::release();
        break;
#endif
#ifdef ENABLE_BACKEND_DX11
    case globals::Backend::DX11:
        hooks_dx11::release();
        break;
#endif
#ifdef ENABLE_BACKEND_DX12
    case globals::Backend::DX12:
        d3d12hook::release();
        break;
#endif
#ifdef ENABLE_BACKEND_VULKAN
    case globals::Backend::Vulkan:
        hooks_vk::release();
        break;
#endif
    default:
        break;
    }

    mousehooks::Remove();

    // Disable and remove all hooks, then uninitialize MinHook
    MH_DisableHook(MH_ALL_HOOKS);
    MH_RemoveHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    DebugLog("[DllMain] Unloading module and exiting thread.\n");
    FreeLibraryAndExitThread(globals::mainModule, 0);
    return 0; // not reached
}

// Public helper to begin uninjecting the DLL
void Uninject()
{
    HANDLE hThread = CreateThread(nullptr, 0, UninjectThread, nullptr, 0, nullptr);
    if (hThread)
        CloseHandle(hThread);
}

static std::mutex g_mReinitHooksGuard;
DWORD WINAPI ReinitializeGraphicalHooks(LPVOID lpParam) {
    std::lock_guard<std::mutex> guard{ g_mReinitHooksGuard };

    DebugLog("[!] Hooks will reinitialize!\n");

    switch (globals::activeBackend)
    {
#ifdef ENABLE_BACKEND_DX9
    case globals::Backend::DX9:
        d3d9hook::release();
        break;
#endif
#ifdef ENABLE_BACKEND_DX10
    case globals::Backend::DX10:
        hooks_dx10::release();
        break;
#endif
#ifdef ENABLE_BACKEND_DX11
    case globals::Backend::DX11:
        hooks_dx11::release();
        break;
#endif
#ifdef ENABLE_BACKEND_DX12
    case globals::Backend::DX12:
        d3d12hook::release();
        break;
#endif
#ifdef ENABLE_BACKEND_VULKAN
    case globals::Backend::Vulkan:
        hooks_vk::release();
        break;
#endif
    default:
        break;
    }

    mousehooks::Remove();
    menu::Remove();

    // Disable and remove all hooks, then uninitialize MinHook
    MH_DisableHook(MH_ALL_HOOKS);

    return 0;
}

// Thread entry: initialize MinHook and start hook setup
static DWORD WINAPI onAttach(LPVOID lpParameter)
{
    DebugLog("[DllMain] onAttach starting.\n");

    // Initialize MinHook
    {
        MH_STATUS mhStatus = MH_Initialize();
        if (mhStatus != MH_OK) {
            DebugLog("[DllMain] MinHook initialization failed: %s\n",
                MH_StatusToString(mhStatus));
            return 1;
        }
        DebugLog("[DllMain] MinHook initialized.\n");
    }

    globals::CallCustomInit();

    WrapperSystem::Init(globals::mainModule);

	// Hook OutputDebugStringA for enhanced logging
    MH_STATUS res = MH_CreateHook(&OutputDebugStringA, reinterpret_cast<LPVOID>(kernel32hook::hookOutputDebugStringA), reinterpret_cast<LPVOID*>(&kernel32hook::oOutputDebugStringA));
    if (res == MH_OK) MH_EnableHook(&OutputDebugStringA);
    DebugLog("[DllMain] Hooked OutputDebugStringA with status %d\n", res);

    // Detect loaded rendering backends and initialize hooks accordingly
    if (globals::preferredBackend != globals::Backend::None)
        TryInitBackend(globals::preferredBackend);
    //else
    //    TryInitializeFrom(globals::Backend::Vulkan);

    // Hook LoadLibraryA/W to catch backends loaded after injection
    HMODULE k32 = GetModuleHandleA("kernel32.dll");
    if (k32) {
        LPVOID addrA = GetProcAddress(k32, "LoadLibraryA");
        LPVOID addrW = GetProcAddress(k32, "LoadLibraryW");
        if (addrA) {
            MH_CreateHook(addrA, reinterpret_cast<LPVOID>(kernel32hook::hookLoadLibraryA), reinterpret_cast<LPVOID*>(&kernel32hook::oLoadLibraryA));
            MH_EnableHook(addrA);
            DebugLog("[DllMain] Hooked LoadLibraryA@%p\n", addrA);
        }
        if (addrW) {
            MH_CreateHook(addrW, reinterpret_cast<LPVOID>(kernel32hook::hookLoadLibraryW), reinterpret_cast<LPVOID*>(&kernel32hook::oLoadLibraryW));
            MH_EnableHook(addrW);
            DebugLog("[DllMain] Hooked LoadLibraryW@%p\n", addrW);
        }
    }

    mousehooks::Init();
    menu::Hook();

    DebugLog("[DllMain] Hook initialization completed.\n");
    return 0;
}

namespace univhook {
  void Attach(HMODULE hModule) {
    console::Alloc();
    DebugLog("[DllMain] DLL_PROCESS_ATTACH: hModule=%p\n", hModule);
    globals::mainModule = hModule;
    // Create a thread for hook setup to avoid blocking loading
    {
      HANDLE thread = CreateThread(
        nullptr, 0,
        onAttach,
        nullptr,
        0,
        nullptr
      );
      if (thread) CloseHandle(thread);
      else DebugLog("[DllMain] Failed to create hook thread: %d\n", GetLastError());
    }
  }

  void Detach() {
    DebugLog("[DllMain] DLL_PROCESS_DETACH. Releasing hooks and uninitializing MinHook.\n");
    switch (globals::activeBackend) {
#ifdef ENABLE_BACKEND_DX9
    case globals::Backend::DX9:
      d3d9hook::release();
      break;
#endif
#ifdef ENABLE_BACKEND_DX10
    case globals::Backend::DX10:
      hooks_dx10::release();
      break;
#endif
#ifdef ENABLE_BACKEND_DX11
    case globals::Backend::DX11:
      hooks_dx11::release();
      break;
#endif
#ifdef ENABLE_BACKEND_DX12
    case globals::Backend::DX12:
      d3d12hook::release();
      break;
#endif
#ifdef ENABLE_BACKEND_VULKAN
    case globals::Backend::Vulkan:
      hooks_vk::release();
      break;
#endif
    default:
      break;
    }
    WrapperSystem::Shutdown();
    mousehooks::Remove();
    menu::Remove();
    MH_DisableHook(MH_ALL_HOOKS);
    MH_RemoveHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    console::Free();
  }
}

#if 0
BOOL WINAPI DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        univhook::Attach(hModule);
        break;

    case DLL_PROCESS_DETACH:
        univhook::Detach();
        break;
    }
    return TRUE;
}
#endif
