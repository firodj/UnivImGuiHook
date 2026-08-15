#include "stdafx.h"

#include <windows.h>
#include <thread>
#include <iostream>
#include <vector>


#include "util.hpp"
#include "console.hpp"


static BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam) {
	const auto isMainWindow = [handle]() {
		return GetWindow(handle, GW_OWNER) == nullptr && IsWindowVisible(handle);
		};

	DWORD pID = 0;
	GetWindowThreadProcessId(handle, &pID);

	if (GetCurrentProcessId() != pID || !isMainWindow() || handle == GetConsoleWindow())
		return TRUE;

	*reinterpret_cast<HWND*>(lParam) = handle;

	return FALSE;
}

namespace util {

	HWND GetProcessWindow() {
		HWND hwnd = nullptr;
		EnumWindows(::EnumWindowsCallback, reinterpret_cast<LPARAM>(&hwnd));

		while (!hwnd) {
			EnumWindows(::EnumWindowsCallback, reinterpret_cast<LPARAM>(&hwnd));
			DebugLog("[!] Waiting for window to appear.\n");
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		char name[128];
		GetWindowTextA(hwnd, name, RTL_NUMBER_OF(name));
		DebugLog("[+] Got window with name: '%s'\n", name);

		return hwnd;
	}

	size_t GetCallStack(char* lpBuffer, size_t Max, size_t depth) {
		size_t current_len = 0;

#if defined(_M_X64)
		// Level 1: Immediate caller of TargetFunction
		void* caller1 = _ReturnAddress();

		// Level 2 & 3: Deeper callers
		void* callers[3];
		USHORT captured = CaptureStackBackTrace(/* FramesToSkip = */ 1, /* FramesToCapture = */ 3, /* BackTrace = */ callers, /* BackTraceHash = */ NULL);

		printf("L1: %p\n", caller1);
		if (captured > 0) printf("L2: %p\n", callers[0]);
		if (captured > 1) printf("L3: %p\n", callers[1]);
#else
		void* reg_ebp;
		__asm mov reg_ebp, ebp; // Get current EBP

		for (int i = 0; i < depth; ++i) {
			reg_ebp = *(void**)reg_ebp; // Move up one frame
			
			void* level_x = *(void**)((BYTE*)reg_ebp + 4);
			const char* fmt = current_len == 0 ? "(0x%08x)" : "|(0x%08x)";
			current_len += snprintf(lpBuffer + current_len, 256 - current_len, fmt, (unsigned int)level_x);
		}
#endif
		return current_len;
	}

	std::string GetFileNameFromHandle(HANDLE hFile) {
		if (hFile == INVALID_HANDLE_VALUE) return "";

		// Request size first (dwFlags 0x01 returns volume path, 0x00 returns device path)
		DWORD dwSize = GetFinalPathNameByHandleW(hFile, NULL, 0, VOLUME_NAME_DOS);
		//DebugLog("[util] GetFinalPathNameByHandleW required size: %lu, hFile: 0x%X\n", dwSize, hFile);
		if (dwSize == 0) return "";
	
		std::vector<WCHAR> buffer(dwSize);
		if (GetFinalPathNameByHandleW(hFile, buffer.data(), dwSize, VOLUME_NAME_DOS) != 0) {
			return wstring_to_utf8_win(std::wstring(buffer.data()));
		}
		return "";
	}

	std::string wstring_to_utf8_win(const std::wstring& wstr) {
		if (wstr.empty()) return std::string();

		// Calculate the required buffer size
		int utf8_size = ::WideCharToMultiByte(
			CP_UTF8,            // CodePage: use UTF-8
			0,                  // Flags
			wstr.c_str(),       // Source wide string
			static_cast<int>(wstr.length()), // Source length
			nullptr,            // Destination buffer (nullptr to get size)
			0,                  // Destination buffer size
			nullptr, nullptr    // Default char and file to use (not needed for UTF-8)
		);

		if (utf8_size == 0) {
			// Handle error (e.g., GetLastError())
			throw std::runtime_error("WideCharToMultiByte failed to get buffer size.");
		}

		std::string utf8_str(utf8_size, '\0');

		// Perform the actual conversion
		int converted_size = ::WideCharToMultiByte(
			CP_UTF8,
			0,
			wstr.c_str(),
			static_cast<int>(wstr.length()),
			&utf8_str[0],       // Destination buffer
			utf8_size,          // Destination buffer size
			nullptr, nullptr
		);

		if (converted_size == 0) {
			throw std::runtime_error("WideCharToMultiByte failed to convert.");
		}

		return utf8_str;
	}

}