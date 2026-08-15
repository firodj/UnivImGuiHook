#pragma once

#include <windows.h>
#include <string>

namespace util {
	HWND GetProcessWindow();
	size_t GetCallStack(char* lpBuffer, size_t Max, size_t depth);
	std::string GetFileNameFromHandle(HANDLE hFile);
	std::string wstring_to_utf8_win(const std::wstring& wstr);
};

