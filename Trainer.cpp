#include "Trainer.h"
#include <iostream>

Trainer::Trainer() : baseAddress(0), savedLocation{0.0f, 0.0f, 0.0f} {}

Trainer::~Trainer() {
    mem.DetachProcess();
}

void Trainer::RegisterCheats() {
    cheats["god_mode"] = { L"God Mode", false, baseAddress + 0x140A1B2C3, { 0x29, 0x73, 0x04 }, { 0x90, 0x90, 0x90 } };
    cheats["inf_health"] = { L"Infinite Health", false, baseAddress + 0x140A1B2D0, { 0x89, 0x83, 0x00, 0x01, 0x00, 0x00 }, { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 } };
    cheats["inf_oxygen"] = { L"Infinite Oxygen", false, baseAddress + 0x140B3C4F0, { 0xF3, 0x0F, 0x11, 0x47, 0x10 }, { 0x90, 0x90, 0x90, 0x90, 0x90 } };
    cheats["inf_ammo"] = { L"Infinite Ammo", false, baseAddress + 0x140C5D6A1, { 0xFF, 0xCE }, { 0x90, 0x90 } };
    cheats["inf_items"] = { L"Infinite Items", false, baseAddress + 0x140D7E8B2, { 0x89, 0x10 }, { 0x90, 0x90 } };
    cheats["stealth"] = { L"Stealth Mode", false, baseAddress + 0x140E9F9C3, { 0x0F, 0x84, 0x00, 0x01, 0x00, 0x00 }, { 0xE9, 0x01, 0x01, 0x00, 0x00, 0x90 } };
    cheats["ship_god"] = { L"Ship God Mode", false, baseAddress + 0x141A2B3C4, { 0xF3, 0x44, 0x11, 0x07 }, { 0x90, 0x90, 0x90, 0x90 } };
    cheats["ship_ammo"] = { L"Ship Inf Ammo", false, baseAddress + 0x141B3C4D5, { 0xFF, 0x88, 0x00, 0x02, 0x00, 0x00 }, { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 } };
    cheats["one_hit"] = { L"One Hit Kill", false, baseAddress + 0x141C4D5E6, { 0x29, 0x7B, 0x30 }, { 0xC7, 0x43, 0x30, 0x00, 0x00, 0x00, 0x00 } };
}

bool Trainer::Initialize(const std::wstring& processName) {
    pName = processName;
    if (mem.AttachProcess(pName)) {
        baseAddress = mem.GetModuleBaseAddress(pName);
        if (baseAddress != 0) {
            RegisterCheats();
            return true;
        }
    }
    return false;
}

bool Trainer::ApplyPatch(const std::string& cheatKey) {
    auto it = cheats.find(cheatKey);
    if (it == cheats.end()) return false;
    DWORD oldProtect = 0;
    size_t size = it->second.patchBytes.size();
    if (mem.ProtectMemory(it->second.address, size, PAGE_EXECUTE_READWRITE, oldProtect)) {
        for (size_t i = 0; i < size; ++i) {
            mem.WriteMemory<unsigned char>(it->second.address + i, it->second.patchBytes[i]);
        }
        mem.ProtectMemory(it->second.address, size, oldProtect, oldProtect);
        return true;
    }
    return false;
}

bool Trainer::RestorePatch(const std::string& cheatKey) {
    auto it = cheats.find(cheatKey);
    if (it == cheats.end()) return false;
    DWORD oldProtect = 0;
    size_t size = it->second.originalBytes.size();
    if (mem.ProtectMemory(it->second.address, size, PAGE_EXECUTE_READWRITE, oldProtect)) {
        for (size_t i = 0; i < size; ++i) {
            mem.WriteMemory<unsigned char>(it->second.address + i, it->second.originalBytes[i]);
        }
        mem.ProtectMemory(it->second.address, size, oldProtect, oldProtect);
        return true;
    }
    return false;
}

void Trainer::ToggleCheat(const std::string& cheatKey) {
    auto it = cheats.find(cheatKey);
    if (it != cheats.end()) {
        it->second.enabled = !it->second.enabled;
        if (it->second.enabled) {
            ApplyPatch(cheatKey);
        } else {
            RestorePatch(cheatKey);
        }
    }
}

bool Trainer::IsCheatEnabled(const std::string& cheatKey) {
    auto it = cheats.find(cheatKey);
    if (it != cheats.end()) return it->second.enabled;
    return false;
}

void Trainer::SetMultiplier(const std::string& cheatKey, float value) {
    uintptr_t targetAddr = baseAddress + 0x142E5F6A1;
    mem.WriteMemory<float>(targetAddr, value);
}

void Trainer::SetValue(const std::string& cheatKey, int value) {
    uintptr_t targetAddr = baseAddress + 0x142F6A7B2;
    mem.WriteMemory<int>(targetAddr, value);
}

void Trainer::SavePosition() {
    uintptr_t playerPtr = mem.ResolvePointerChain(baseAddress + 0x143A1B2C3, { 0x8, 0x18, 0x30 });
    if (playerPtr != 0) {
        savedLocation = mem.ReadMemory<Vector3>(playerPtr + 0x90);
    }
}

void Trainer::LoadPosition() {
    if (savedLocation.x == 0.0f && savedLocation.y == 0.0f && savedLocation.z == 0.0f) return;
    uintptr_t playerPtr = mem.ResolvePointerChain(baseAddress + 0x143A1B2C3, { 0x8, 0x18, 0x30 });
    if (playerPtr != 0) {
        mem.WriteMemory<Vector3>(playerPtr + 0x90, savedLocation);
    }
}

void Trainer::TeleportToMarker() {
    uintptr_t markerPtr = mem.ResolvePointerChain(baseAddress + 0x144B2C3D4, { 0x10, 0x20, 0x58 });
    if (markerPtr != 0) {
        Vector3 markerPos = mem.ReadMemory<Vector3>(markerPtr + 0x110);
        uintptr_t playerPtr = mem.ResolvePointerChain(baseAddress + 0x143A1B2C3, { 0x8, 0x18, 0x30 });
        if (playerPtr != 0 && markerPos.z != 0.0f) {
            markerPos.z += 2.0f;
            mem.WriteMemory<Vector3>(playerPtr + 0x90, markerPos);
        }
    }
}

void Trainer::ModifyItemAmount(int amount) {
    uintptr_t inventoryPtr = mem.ResolvePointerChain(baseAddress + 0x145C3D4E5, { 0x28, 0x40 });
    if (inventoryPtr != 0) {
        mem.WriteMemory<int>(inventoryPtr + 0x14, amount);
    }
}

void Trainer::Update() {
    if (GetAsyncKeyState(VK_NUMPAD1) & 0x8000) { ToggleCheat("god_mode"); Sleep(250); }
    if (GetAsyncKeyState(VK_NUMPAD2) & 0x8000) { ToggleCheat("inf_health"); Sleep(250); }
    if (GetAsyncKeyState(VK_NUMPAD3) & 0x8000) { ToggleCheat("inf_oxygen"); Sleep(250); }
    if (GetAsyncKeyState(VK_NUMPAD4) & 0x8000) { ToggleCheat("inf_ammo"); Sleep(250); }
    if (GetAsyncKeyState(VK_NUMPAD5) & 0x8000) { ToggleCheat("inf_items"); Sleep(250); }
    if (GetAsyncKeyState(VK_NUMPAD6) & 0x8000) { ToggleCheat("stealth"); Sleep(250); }
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
        if (GetAsyncKeyState(VK_NUMPAD1) & 0x8000) { ToggleCheat("ship_god"); Sleep(250); }
        if (GetAsyncKeyState(VK_NUMPAD2) & 0x8000) { ToggleCheat("ship_ammo"); Sleep(250); }
        if (GetAsyncKeyState(VK_NUMPAD3) & 0x8000) { ToggleCheat("one_hit"); Sleep(250); }
    }
    if (GetAsyncKeyState(VK_MENU) & 0x8000) {
        if (GetAsyncKeyState(VK_NUMPAD1) & 0x8000) { SetValue("money", 1000000); Sleep(250); }
        if (GetAsyncKeyState(VK_NUMPAD2) & 0x8000) { SetMultiplier("money_mult", 5.0f); Sleep(250); }
        if (GetAsyncKeyState(VK_NUMPAD3) & 0x8000) { ModifyItemAmount(9999); Sleep(250); }
        if (GetAsyncKeyState(VK_MULTIPLY) & 0x8000) { SavePosition(); Sleep(250); }
        if (GetAsyncKeyState(VK_ADD) & 0x8000) { LoadPosition(); Sleep(250); }
        if (GetAsyncKeyState(VK_SUBTRACT) & 0x8000) { TeleportToMarker(); Sleep(250); }
    }
}