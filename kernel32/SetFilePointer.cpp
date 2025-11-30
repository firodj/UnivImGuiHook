#include "kernel32hook.hpp"

#include "../util.hpp"
#include "../console.hpp"
#include "../globals.hpp"

namespace kernel32hook {
	SetFilePointerFn oSetFilePointer = nullptr;
	DWORD WINAPI hookSetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod) {
#define BUFFER_SIZE 256
		char callerTrace[BUFFER_SIZE] = ""; // Initialize with an empty string
		std::string filename;

		if (!globals::HasRefForDisableFileAccessLog()) {
			size_t current_len = util::GetCallStack(callerTrace, BUFFER_SIZE, 3);

			filename = util::GetFileNameFromHandle(hFile);
		}
		DWORD res = oSetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
		
		if (!globals::HasRefForDisableFileAccessLog()) {
			DebugLog("[SetFilePointer %s, result=0x%x] hFile: 0x%p(%s), lDistanceToMove: %ld, lpDistanceToMoveHigh: 0x%p, dwMoveMethod: %lu\n",
				callerTrace, res, hFile, filename.c_str(), lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
		}

		return res;
	}
}