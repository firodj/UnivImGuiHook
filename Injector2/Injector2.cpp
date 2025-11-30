// Injector2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// https://everthessel.nl/blog/dll-injection-function-hooking/

#include    <stdio.h>
#include    <string>
#include    <windows.h>
#include    <iostream>
#include <memory>
#include <vector>

struct Option {
    std::string exe;
    std::string dll;

    std::string exe_name() {
        size_t last_slash_pos = exe.find_last_of("/\\");

        // If a slash is found, extract the substring after it
        if (last_slash_pos != std::string::npos) {
            return exe.substr(last_slash_pos + 1);
        }
        
        // If no slash is found, the entire string is the base filename
        return exe;
    }
};

Option parse_args(std::vector<std::string> args) {
    Option opt;
    bool expect_dll = false;
    bool expect_exe = false;
    for (auto arg : args) {
        if (arg == "-dll") {
            expect_dll = true;
        }
        else if (arg == "-exe") {
            expect_exe = true;
        }
        else if (expect_dll) {
            expect_dll = false;
            opt.dll = arg;
        }
        else if (expect_exe) {
            expect_exe = false;
            opt.exe = arg;
        }
        else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            break;
        }
    }
    return opt;
}

DWORD getEntryPointFromPEfile(const char* peFilePath) {
    FILE* file = nullptr;
    errno_t err;

    err = fopen_s(&file, peFilePath, "rb");
    if (file == NULL || err != 0) {
        std::cerr << "Failed to open file: " << peFilePath << std::endl;
        return 0;
	}
    // Read the DOS header
    IMAGE_DOS_HEADER dosHeader;
    fread(&dosHeader, sizeof(IMAGE_DOS_HEADER), 1, file);
    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
        std::cerr << "Invalid DOS signature." << std::endl;
        fclose(file);
        return 0;
    }
    // Move to the NT headers
    fseek(file, dosHeader.e_lfanew, SEEK_SET);
    // Read the NT headers
    IMAGE_NT_HEADERS ntHeaders;
    fread(&ntHeaders, sizeof(IMAGE_NT_HEADERS), 1, file);
    if (ntHeaders.Signature != IMAGE_NT_SIGNATURE) {
        std::cerr << "Invalid NT signature." << std::endl;
        fclose(file);
        return 0;
    }
    // Get the entry point address
    DWORD entryPoint = ntHeaders.OptionalHeader.AddressOfEntryPoint;
    fclose(file);

	// convert to virtual address by adding the image base
	entryPoint += ntHeaders.OptionalHeader.ImageBase;
    
	return entryPoint;
}

void PrintNextExecutionAddress(HANDLE hThread) {
    // 1. Create a CONTEXT structure
    CONTEXT context;
    SecureZeroMemory(&context, sizeof(CONTEXT));

    // 2. Specify that we only want to retrieve the control registers (EIP, ESP, etc.)
    context.ContextFlags = CONTEXT_CONTROL;

    // 3. Retrieve the context from the suspended thread
    if (GetThreadContext(hThread, &context)) {
        // Under 32-bit Windows, the register is context.Eip
        std::cout << "Thread will resume execution at address: 0x"
            << std::hex << context.Eip << std::dec << std::endl;
    }
    else {
        std::cerr << "Failed to get thread context. Error: " << GetLastError() << std::endl;
    }
}

int main(int argc, char* argv[])
{
    std::cout << "Program name: " << argv[0] << std::endl;
    std::vector<std::string> args_as_strings;
    for (int i = 1; i < argc; ++i) { 
        args_as_strings.push_back(std::string(argv[i]));
    }
    auto opt = parse_args(args_as_strings);

    // Path to our application
    const char* exe = opt.exe.c_str();
    const char* dll = opt.dll.c_str();
    int dwDelay = 1000; // 1000 millis

    // Arguments for our application, including name of the executable itself
    auto exe_name = opt.exe_name();
    std::vector<char> args(exe_name.begin(), exe_name.end());
    args.push_back('\0');

	DWORD entryPointAddress = getEntryPointFromPEfile(exe);
    if (entryPointAddress == 0) {
        std::cerr << "Failed to get entry point from PE file." << std::endl;
        return 1;
	}

	std::cout << "Found entryPointAddress: 0x" << std::hex << entryPointAddress << std::dec << "\n";

    // Address to the main function of our application
    LPVOID entrypoint = (LPVOID)entryPointAddress; // was: 0x0057C184;

    // Holds properties of the started process
    STARTUPINFOA StartupInfo = { 0 };
    // For backwards/forwards compatibility Windows requires us to fill in the size of the StartupInfo instance
    StartupInfo.cb = sizeof(StartupInfo);

    // Also holds properties of the started process
    PROCESS_INFORMATION ProcessInformation;

    // Variables for tracking the memory protection of the process
    DWORD originalProtection;
    DWORD unusedProtection;

    // Holds the original bytes that we will replace with an infinite loop
    char originalBytes[2];
    // Infinite loop instruction
    static const char infiniteLoop[2] = { '\xEB', '\xFE' };
    // Generic size variable, stores the number of bytes written/read using ReadProcessMemory/WriteProcessMemory
    SIZE_T size;
    // Handle of the loaded DLL
    HANDLE LoadDLL;
    // Handle of the Kernel32 module
    HMODULE Kernel32;
    // Full path to the DLL
    char dllPath[_MAX_PATH];
    // Pointer to the DLL path in the process
    void* remoteDllPath;

    // Spawn the application and immediately suspend it
    if (CreateProcessA(
        exe,
        args.data(),
        NULL,
        NULL,
        FALSE,
        CREATE_SUSPENDED,
        NULL,
        NULL,
        &StartupInfo,
        &ProcessInformation)) {
        HANDLE hProcess(ProcessInformation.hProcess);

        try {
            // Entry point of the application
            LPVOID entry = entrypoint;

            // Make the memory of the process writeable
            VirtualProtectEx(hProcess, entry, 2, PAGE_EXECUTE_READWRITE, &originalProtection);

            // Save the original bytes at the entry point and replace them with a JMP $-2 / infinite loop
            ReadProcessMemory(hProcess, entry, originalBytes, 2, &size);
            WriteProcessMemory(hProcess, entry, infiniteLoop, 2, &size);
            VirtualProtectEx(hProcess, entry, 2, originalProtection, &unusedProtection);

            // Resume the process
            ResumeThread(ProcessInformation.hThread);

            // Wait until the process has reached the infinite loop
            for (;;) {
                // Check if the Instruction Pointer points to our infinite loop
                CONTEXT context;
                context.ContextFlags = CONTEXT_CONTROL;
                GetThreadContext(ProcessInformation.hThread, &context);

                // If yes, exit the loop
                if (context.Eip == (DWORD)entry) {
                    break;
                }

                // Else, sleep for a moment and then try again
                Sleep(100);
            }

            // Get the full path to the DLL
            GetFullPathNameA(dll, _MAX_PATH, dllPath, NULL);

            // Allocate memory for the DLL path in the application process
            remoteDllPath = VirtualAllocEx(hProcess, NULL, sizeof(dllPath), MEM_COMMIT, PAGE_READWRITE);
            if (remoteDllPath == nullptr) {
                throw;
            }

            // Write the DLL path to the allocated memory
            WriteProcessMemory(hProcess, remoteDllPath, (void*)dllPath, sizeof(dllPath), NULL);

            // Get a handle to the Kernel32 module
            Kernel32 = GetModuleHandleA("Kernel32");
            if (Kernel32 == nullptr) {
                throw;
            }

            // Call LoadLibraryA from the application process and pass the DLL path as a parameter
			std::cout << "Attempt to load DLL: " << dllPath << "\n";
            LoadDLL = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(Kernel32, "LoadLibraryA"), remoteDllPath, 0, NULL);
            if (LoadDLL == nullptr) {
                throw;
            }

            // Wait for the DLL to load and return.
            WaitForSingleObject(LoadDLL, INFINITE);
            CloseHandle(LoadDLL);

            // Free the memory used for storing the DLL path in the application process
            VirtualFreeEx(hProcess, remoteDllPath, 0, MEM_RELEASE);

            // Suspend the process and make its memory writeable again
            SuspendThread(ProcessInformation.hThread);
            VirtualProtectEx(hProcess, entry, 2, PAGE_EXECUTE_READWRITE, &originalProtection);

            // Restore the original bytes
            WriteProcessMemory(hProcess, entry, originalBytes, 2, &size);

            // Restore the original memory protection
            VirtualProtectEx(hProcess, entry, 2, originalProtection, &unusedProtection);

            // Delay to give hooks complete at least.
            Sleep(dwDelay);

            // Resume the patched application
            ResumeThread(ProcessInformation.hThread);

            std::cout << "Successfully injected DLL\n";
            return 0;
        }
        catch (...) {
            // An error occurred, kill the spawned process
            TerminateProcess(hProcess, -1);
            std::cout << "Error while injecting DLL\n";
        }
    }
    else {
        std::cout << "Application not found\n";
    }
    
    std::cout << "Press any key..." << std::endl; std::cin.get();
    return -1;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
