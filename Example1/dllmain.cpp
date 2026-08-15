// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "univhook.hpp"
#include "globals.hpp"
#include "console.hpp"

void ExampleInit() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    const char* base = strrchr(path, '\\');
    base = base ? base + 1 : path;

    DebugLog("[DLLMain] Found Base Module: %p\n", base);
}

void ExampleRender() {
    static bool isOpen = true;

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    ImGui::SetNextWindowSize(ImVec2(450, 600), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(25, 25), ImGuiCond_FirstUseEver);

    ImGui::Begin("Example", &isOpen, flags);

    ImGui::Text("Press <INSERT> to toggle menu.");

    ImGui::End();
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        globals::SetCustomInit(ExampleInit);
        globals::SetCustomRender(ExampleRender);
        univhook::Attach(hModule);
        break;

    case DLL_PROCESS_DETACH:
        univhook::Detach();
        break;        
    }
    return TRUE;
}

