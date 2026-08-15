#include "kernel32hook.hpp"

#include "../util.hpp"
#include "../console.hpp"

namespace kernel32hook {    
    OutputDebugStringAFn oOutputDebugStringA = nullptr;

    void WINAPI hookOutputDebugStringA(LPCSTR lpOutputString) {
#define BUFFER_SIZE 256
        char buffer[BUFFER_SIZE] = ""; // Initialize with an empty string
        size_t current_len = util::GetCallStack(buffer, BUFFER_SIZE, 3);

        DebugLog("[Output %s] %s\n", buffer, lpOutputString);
        //oOutputDebugStringA(lpOutputString);
    }

}
