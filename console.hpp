#pragma once

namespace console {
    void Alloc();
    void Free();
    //extern FILE* pLogFile;
	  void Disable();
    void DrawConsole();
};

void DebugLog(const char* fmt, ...);
void DebugLogV(const char* fmt, va_list args);