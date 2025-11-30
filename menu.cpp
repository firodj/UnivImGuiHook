#include "stdafx.h"

#include "console.hpp"
#include "globals.hpp"
#include "menu.hpp"
#include "kernel32/kernel32hook.hpp"

namespace menu {
    bool isOpen = true;
    float test = 0.0f;
    static bool noTitleBar = false;

    void Init(HWND hWnd) {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport

        ImGui::StyleColorsDark();
        ImGui_ImplWin32_Init(hWnd);
        DebugLog("[menu] ImGui initialized.\n");
	}

    // Render all ImGui things. The hook may check the isOpen first before calling this.
    void Render() {
        // Begin frame UI setup
        //DebugLog("[menu] Rendering menu. isOpen=%d, test=%.2f\n", isOpen, test);

        ImGuiIO& io = ImGui::GetIO();
        io.MouseDrawCursor = isOpen;

        if (!isOpen) return;

        // Style setup (one-time)
        static bool styled = false;
        if (!styled) {
            ImGui::StyleColorsDark();
            ImVec4* colors = ImGui::GetStyle().Colors;
            // Custom color palette
            colors[ImGuiCol_WindowBg] = ImVec4(0, 0, 0, 0.8f);
            colors[ImGuiCol_Header] = ImVec4(0.2f, 0.2f, 0.2f, 0.8f);
            colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.3f, 0.3f, 0.8f);
            colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.4f);
            colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.0f);
            colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.0f);
            styled = true;
            DebugLog("[menu] Style applied.\n");
        }

        console::DrawConsole();
        globals::CallCustomRender();

        // Window flags
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
        ImGui::SetNextWindowSize(ImVec2(450, 600), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(25, 25), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("ImGui Menu", &globals::enableSimpleMenu, flags)) {
		        ImGui::Text("Press <INSERT> to toggle menu.");
            ImGui::Text("WindowProc=0x%X", globals::sOriginalWndProc);

            if (ImGui::CollapsingHeader("MENU")) {
                if (ImGui::TreeNode("SUB MENU")) {
                    ImGui::Text("Text Test");
                    if (ImGui::Button("Button Test")) {
                        DebugLog("[menu] Button Test clicked.\n");
                    }
                    if (ImGui::Checkbox("No Title Bar", &noTitleBar)) {
                        DebugLog("[menu] Checkbox No Title Bar toggled. flags=0x%X\n", flags);
                    }
                    ImGui::SliderFloat("Slider Test", &test, 1.0f, 100.0f);
                    ImGui::Text("Slider value=%.2f", test);
                    ImGui::TreePop();
                }
            }
        }
        ImGui::End();
    }

    void Hook() {
        MH_STATUS res;

        if (globals::enableHookReadFile) {
            res = MH_CreateHook(&ReadFile,
                reinterpret_cast<LPVOID>(kernel32hook::hookReadFile),
                reinterpret_cast<LPVOID*>(&kernel32hook::oReadFile)
		            );
            if (res == MH_OK) {
                MH_EnableHook(&ReadFile);
		        }
		        DebugLog("[menu] MH_CreateHookApi ReadFile result: %d\n", res);
        }

        res = MH_CreateHook(&CreateFileA,
            reinterpret_cast<LPVOID>(kernel32hook::hookCreateFileA),
			      reinterpret_cast<LPVOID*>(&kernel32hook::oCreateFileA)
            );
        if (res == MH_OK) {
            MH_EnableHook(&CreateFileA);
        }
		    DebugLog("[menu] MH_CreateHookApi CreateFileA result: %d\n", res);

        res = MH_CreateHook(&SetFilePointer,
            reinterpret_cast<LPVOID>(kernel32hook::hookSetFilePointer),
            reinterpret_cast<LPVOID*>(&kernel32hook::oSetFilePointer)
		    );
        if (res == MH_OK) {
            MH_EnableHook(&SetFilePointer);
        }
		    DebugLog("[menu] MH_CreateHookApi SetFilePointer result: %d\n", res);
    }

    void Remove() {

    }
}
