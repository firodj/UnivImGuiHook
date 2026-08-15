#pragma once

#include <windows.h>
#include <type_traits>
#include <string>
#include <map>

namespace kernel32hook {
	typedef std::add_pointer_t<void WINAPI(LPCSTR lpOutputString)> OutputDebugStringAFn;
	typedef std::add_pointer_t<BOOL WINAPI(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED)> ReadFileFn;
    typedef std::add_pointer_t<HANDLE WINAPI(
        LPCSTR                lpFileName,
        DWORD                 dwDesiredAccess,
        DWORD                 dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD                 dwCreationDisposition,
        DWORD                 dwFlagsAndAttributes,
        HANDLE                hTemplateFile
    )> CreateFileAFn;
    typedef std::add_pointer_t<DWORD WINAPI(
        HANDLE hFile,
        LONG   lDistanceToMove,
        PLONG  lpDistanceToMoveHigh,
        DWORD  dwMoveMethod
    )> SetFilePointerFn;

    // Pointers to original LoadLibrary functions
    typedef std::add_pointer_t<HMODULE WINAPI(LPCSTR)> LoadLibraryA_t;
    typedef std::add_pointer_t<HMODULE WINAPI(LPCWSTR)> LoadLibraryW_t;

    extern LoadLibraryA_t oLoadLibraryA;
    extern LoadLibraryW_t oLoadLibraryW;
	extern ReadFileFn oReadFile;
    extern CreateFileAFn oCreateFileA;
    extern OutputDebugStringAFn oOutputDebugStringA;
	extern SetFilePointerFn oSetFilePointer;

	void WINAPI hookOutputDebugStringA(LPCSTR lpOutputString);

    HANDLE WINAPI hookCreateFileA(
        LPCSTR                lpFileName,
        DWORD                 dwDesiredAccess,
        DWORD                 dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD                 dwCreationDisposition,
        DWORD                 dwFlagsAndAttributes,
        HANDLE                hTemplateFile
	);
	BOOL WINAPI hookReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
    HMODULE WINAPI hookLoadLibraryA(LPCSTR lpLibFileName);
    HMODULE WINAPI hookLoadLibraryW(LPCWSTR lpLibFileName);
    DWORD WINAPI hookSetFilePointer(
        HANDLE hFile,
        LONG   lDistanceToMove,
        PLONG  lpDistanceToMoveHigh,
        DWORD  dwMoveMethod
	);
    
};

