#include <iostream>
#include <chrono>
#include <thread>
#include "Trainer.h"

void RenderInterface(Trainer& trainer) {
    system("cls");
    std::wcout << L"==================================================" << std::endl;
    std::wcout << L"    ASSASSIN'S CREED BLACK FLAG RESYNCED TRAINER  " << std::endl;
    std::wcout << L"==================================================" << std::endl;
    std::wcout << L"[NUM1] God Mode: " << (trainer.IsCheatEnabled("god_mode") ? L"ON" : L"OFF") << std::endl;
    std::wcout << L"[NUM2] Infinite Health: " << (trainer.IsCheatEnabled("inf_health") ? L"ON" : L"OFF") << std::endl;
    std::wcout << L"[NUM3] Infinite Oxygen: " << (trainer.IsCheatEnabled("inf_oxygen") ? L"ON" : L"OFF") << std::endl;
    std::wcout << L"[NUM4] Infinite Ammo: " << (trainer.IsCheatEnabled("inf_ammo") ? L"ON" : L"OFF") << std::endl;
    std::wcout << L"[NUM5] Infinite Items: " << (trainer.IsCheatEnabled("inf_items") ? L"ON" : L"OFF") << std::endl;
    std::wcout << L"[NUM6] Stealth Mode: " << (trainer.IsCheatEnabled("stealth") ? L"ON" : L"OFF") << std::endl;
    std::wcout << L"--------------------------------------------------" << std::endl;
    std::wcout << L"[CTRL+NUM1] Ship God Mode: " << (trainer.IsCheatEnabled("ship_god") ? L"ON" : L"OFF") << std::endl;
    std::wcout << L"[CTRL+NUM2] Ship Inf Ammo: " << (trainer.IsCheatEnabled("ship_ammo") ? L"ON" : L"OFF") << std::endl;
    std::wcout << L"[CTRL+NUM3] One Hit Kill: " << (trainer.IsCheatEnabled("one_hit") ? L"ON" : L"OFF") << std::endl;
    std::wcout << L"--------------------------------------------------" << std::endl;
    std::wcout << L"[ALT+NUM1] Add 1,000,000 Reals" << std::endl;
    std::wcout << L"[ALT+NUM2] Set Money Multiplier to 5x" << std::endl;
    std::wcout << L"[ALT+NUM3] Set Selected Items to 9999" << std::endl;
    std::wcout << L"[ALT+*] Save Position | [ALT++] Load Position" << std::endl;
    std::wcout << L"[ALT+-] Teleport to Custom Map Marker" << std::endl;
    std::wcout << L"================================================--" << std::endl;
    std::wcout << L"Press [END] to safely exit trainer application." << std::endl;
}

int main() {
    Trainer appTrainer;
    std::wstring targetProcess = L"ACBlackFlag.exe";
    
    std::wcout << L"Searching for game client execution context..." << std::endl;
    
    while (!appTrainer.Initialize(targetProcess)) {
        if (GetAsyncKeyState(VK_END) & 0x8000) {
            return 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::wcout << L"Successfully bound to target memory runtime context." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    auto lastRefresh = std::chrono::steady_clock::now();
    RenderInterface(appTrainer);

    while (!(GetAsyncKeyState(VK_END) & 0x8000)) {
        appTrainer.Update();
        
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRefresh).count() > 300) {
            RenderInterface(appTrainer);
            lastRefresh = now;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}