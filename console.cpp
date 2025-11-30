#include "stdafx.h"

#include <string>

#include "console.hpp"
#include "globals.hpp"

namespace console {
	  FILE* pLogFile = nullptr;
    bool bDisabled = false;
    std::vector<std::string> Lines;
    bool ScrollToBottom = true;

    void OpenLogFile() {
		    const char* fname = "Universal-ImGui-Hook.log";
		    if (fopen_s(&pLogFile, fname, "w") == 0) {
            // Successfully opened
            printf("[console] Log file streamed to %s.\n", fname);
        }
        else {
            pLogFile = nullptr;
            printf("[console] Failed to open log file for writing.\n");
		    }
    }

    void CloseLogFile() {
        if (pLogFile) {
            fclose(pLogFile);
            pLogFile = nullptr;
        }
    }

    void Alloc() {
        AllocConsole();

        SetConsoleOutputCP(932); // Shift-JIS
        SetConsoleTitleA("Debug Console");

        freopen_s(reinterpret_cast<FILE**>(stdin), "conin$", "r", stdin);
        freopen_s(reinterpret_cast<FILE**>(stdout), "conout$", "w", stdout);

        ::ShowWindow(GetConsoleWindow(), SW_SHOW);
        OpenLogFile();
    }

    void Free() {
        fclose(stdin);
        fclose(stdout);

        //if (globals::shuttingDown) {
        ::ShowWindow(GetConsoleWindow(), SW_HIDE);
        //}
        //else {
        FreeConsole();
        //}
        CloseLogFile();
    }

    void Disable() {
        bDisabled = true;
    }

    void DrawConsole() {
        ImGui::Begin("Engine Log");

        // Use a child window for the scrolling region
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);

        // CLIPPER: Only render lines that are actually on screen
        ImGuiListClipper clipper;
        clipper.Begin(Lines.size());
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                ImGui::TextUnformatted(Lines[i].c_str());
            }
        }

        if (ScrollToBottom && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ScrollToBottom = false;
        ImGui::EndChild();
        ImGui::End();
    }
};

// Helper macro for debug logging via DebugView
void DebugLog(const char* fmt, ...) {
    if (!globals::enableDebugLog) {
        return;
    }
    
    //char buf[512];
    va_list args;
    va_start(args, fmt);
    DebugLogV(fmt, args);
    //vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    //OutputDebugStringA(buf);
}

void DebugLogV(const char* fmt, va_list args) {
    // 1. Preallocate a local buffer for the formatted string
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, args);

    // 2. Handle Newlines / Virtual Buffer Logic
    std::string fullEntry(buf);
    size_t start = 0, end = 0;

    // Split by newline so every line is a separate entry in our "virtual buffer"
    while ((end = fullEntry.find('\n', start)) != std::string::npos) {
        console::Lines.push_back(fullEntry.substr(start, end - start));
        start = end + 1;
    }

    // Add the remaining part of the string if no trailing newline
    if (start < fullEntry.length()) {
        console::Lines.push_back(fullEntry.substr(start));
    }

    console::ScrollToBottom = true; // Flag to snap view to latest logs

    // 3. Send to file
    if (console::pLogFile) {
        vfprintf(console::pLogFile, fmt, args);
        // Flush the stream
        fflush(console::pLogFile); // <--- This forces the data to be written
    }

    // 4. Send to default Console Window
    if (console::bDisabled) {
        return;
	}
    vprintf(fmt, args);
	//OutputDebugStringA(buf);
}
