#include "kernel32hook.hpp"
#include "../console.hpp"
#include "../util.hpp"
#include "../globals.hpp"
#include <string>

namespace kernel32hook {
    ReadFileFn oReadFile = nullptr;

    BOOL WINAPI hookReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
        #define BUFFER_SIZE 256
        char callerTrace[BUFFER_SIZE] = ""; // Initialize with an empty string
        std::string filename;

        if (!globals::HasRefForDisableFileAccessLog()) {
            size_t current_len = util::GetCallStack(callerTrace, BUFFER_SIZE, 3);

           
            filename = util::GetFileNameFromHandle(hFile);
        }
		
        BOOL res = oReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);

        if (!globals::HasRefForDisableFileAccessLog()) {
            int nNumberOfBytesRead = -1;
            if (lpNumberOfBytesRead) nNumberOfBytesRead = *lpNumberOfBytesRead;

            DebugLog("[ReadFile %s, result=0x%x] hFile: 0x%p(%s), lpBuffer: 0x%p, nNumberOfBytesToRead: %lu, nNumberOfBytesRead: %d, lpOverlapped: 0x%p\n",
                callerTrace, res, hFile, filename.c_str(), lpBuffer, nNumberOfBytesToRead, nNumberOfBytesRead, lpOverlapped);
        }

        return res;
    }

}