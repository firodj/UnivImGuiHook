#include "kernel32hook.hpp"

#include "../console.hpp"
#include "../util.hpp"
#include "../globals.hpp"

namespace kernel32hook {
    CreateFileAFn oCreateFileA = nullptr;

    HANDLE WINAPI hookCreateFileA(
        LPCSTR                lpFileName,
        DWORD                 dwDesiredAccess,
        DWORD                 dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD                 dwCreationDisposition,
        DWORD                 dwFlagsAndAttributes,
        HANDLE                hTemplateFile)
    {
        #define BUFFER_SIZE 256
        char callerTrace[BUFFER_SIZE] = ""; // Initialize with an empty string

        if (!globals::HasRefForDisableFileAccessLog()) {

            size_t current_len = util::GetCallStack(callerTrace, BUFFER_SIZE, 3);
        }
        
        HANDLE res = oCreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    
        if (!globals::HasRefForDisableFileAccessLog()) {
            DebugLog("[CreateFileA %s, result=0x%X] lpFileName: %s, dwDesiredAccess: %lu, dwShareMode: %lu, lpSecurityAttributes: 0x%p, dwCreationDisposition: %lu, dwFlagsAndAttributes: %lu, hTemplateFile: 0x%p\n",
                callerTrace, res, lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
        }

        return res;
    }
}