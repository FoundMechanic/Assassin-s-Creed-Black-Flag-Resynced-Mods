#include "MemoryManager.h"
#include <tlhelp32.h>
#include <iostream>

MemoryManager::MemoryManager() : processId(0), hProcess(nullptr) {}

MemoryManager::~MemoryManager() {
    DetachProcess();
}

bool MemoryManager::VerifyHandle() {
    if (hProcess == nullptr || hProcess == INVALID_HANDLE_VALUE) return false;
    DWORD exitCode = 0;
    if (GetExitCodeProcess(hProcess, &exitCode)) {
        return exitCode == STILL_ACTIVE;
    }
    return false;
}

bool MemoryManager::AttachProcess(const std::wstring& processName) {
    DetachProcess();
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;

    bool found = false;
    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (processName == pe32.szExeFile) {
                processId = pe32.th32ProcessID;
                hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
                found = true;
                break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
    if (found && hProcess != nullptr) {
        return CacheModules();
    }
    return false;
}

void MemoryManager::DetachProcess() {
    if (hProcess != nullptr && hProcess != INVALID_HANDLE_VALUE) {
        CloseHandle(hProcess);
        hProcess = nullptr;
    }
    processId = 0;
    moduleCache.clear();
}

bool MemoryManager::CacheModules() {
    if (processId == 0) return false;
    moduleCache.clear();
    MODULEENTRY32 me32;
    me32.dwSize = sizeof(MODULEENTRY32);
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;

    if (Module32First(hSnapshot, &me32)) {
        do {
            ProcessModuleInfo info;
            info.moduleName = me32.szModule;
            info.baseAddress = reinterpret_cast<uintptr_t>(me32.modBaseAddr);
            info.moduleSize = static_cast<size_t>(me32.modBaseSize);
            moduleCache.push_back(info);
        } while (Module32Next(hSnapshot, &me32));
    }
    CloseHandle(hSnapshot);
    return !moduleCache.empty();
}

uintptr_t MemoryManager::GetModuleBaseAddress(const std::wstring& moduleName) {
    for (const auto& mod : moduleCache) {
        if (mod.moduleName == moduleName) {
            return mod.baseAddress;
        }
    }
    return 0;
}

uintptr_t MemoryManager::ResolvePointerChain(uintptr_t baseAddress, const std::vector<ptrdiff_t>& offsets) {
    uintptr_t currentAddress = baseAddress;
    for (size_t i = 0; i < offsets.size(); ++i) {
        currentAddress = ReadMemory<uintptr_t>(currentAddress);
        if (currentAddress == 0) return 0;
        currentAddress += offsets[i];
    }
    return currentAddress;
}

uintptr_t MemoryManager::FindPattern(const std::wstring& moduleName, const char* pattern, const char* mask) {
    uintptr_t base = GetModuleBaseAddress(moduleName);
    size_t size = 0;
    for (const auto& mod : moduleCache) {
        if (mod.moduleName == moduleName) {
            size = mod.moduleSize;
            break;
        }
    }
    if (base == 0 || size == 0) return 0;
    std::vector<char> buffer(size);
    SIZE_T bytesRead = 0;
    if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(base), buffer.data(), size, &bytesRead)) return 0;

    size_t patternLength = strlen(mask);
    for (size_t i = 0; i < size - patternLength; ++i) {
        bool found = true;
        for (size_t j = 0; j < patternLength; ++j) {
            if (mask[j] == 'x' && pattern[j] != buffer[i + j]) {
                found = false;
                break;
            }
        }
        if (found) return base + i;
    }
    return 0;
}