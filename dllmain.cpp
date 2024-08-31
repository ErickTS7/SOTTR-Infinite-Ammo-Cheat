#include <windows.h>
#include <iostream>
#include <Minhook.h>
#include "Globals.h" 

#if _WIN64
#pragma comment (lib, "libMinHook.x64.lib")
#else
#pragma comment (lib, "libMinHook.x86.lib")
#endif

HINSTANCE DllHandle;

// Typedef
typedef void(__cdecl* reload)(unsigned int a);

// Módulo base
uintptr_t GameAssembly;

// Calcular o endereço da função original
reload CalculateFuncTarget() {
    GameAssembly = reinterpret_cast<uintptr_t>(GetModuleHandleA("SOTTR.exe"));
    if (GameAssembly == 0) {
        std::cerr << "Failed to get module handle for SOTTR.exe" << std::endl;
        return nullptr;
    }
    return reinterpret_cast<reload>(GameAssembly + 0x6B3B65);
}

extern "C" void GetInfiniteAmmo();

DWORD __stdcall EjectThread(LPVOID lpParameter) {
    Sleep(100);
    FreeLibraryAndExitThread(DllHandle, 0);
    return 0;
}

void shutdown(FILE* fp, std::string reason) {
    MH_Uninitialize();
    std::cout << reason << std::endl;
    Sleep(8000);
    if (fp != nullptr)
        fclose(fp);
    FreeConsole();
    CreateThread(0, 0, EjectThread, 0, 0, 0);
    return;
}

DWORD WINAPI Menue(LPVOID lpParameter) {
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    std::cout << "DLL Injected!" << std::endl;

    MH_STATUS status = MH_Initialize();
    if (status != MH_OK) {
        shutdown(fp, "Minhook init failed!");
        return 0;
    }

    reload FuncTarget = CalculateFuncTarget();
    if (!FuncTarget) {
        shutdown(fp, "Failed to calculate function target address!");
        return 1;
    }

    status = MH_CreateHook(reinterpret_cast<void**>(FuncTarget), &GetInfiniteAmmo, reinterpret_cast<void**>(&OAmmo));
    if (status != MH_OK) {
        std::cout << "CreateHook failed with error: " << MH_StatusToString(status) << std::endl;
        shutdown(fp, "CreateHook failed!");
        return 1;
    }

    bool enableAmmo = false;

    while (true) {
        if (GetAsyncKeyState(VK_INSERT) & 1) {
            enableAmmo = !enableAmmo;
            if (enableAmmo) {
                std::cout << "Infinite Ammo Enabled" << std::endl;
                status = MH_EnableHook(reinterpret_cast<void**>(FuncTarget));
                if (status != MH_OK && status != MH_ERROR_ENABLED) {  
                    std::cout << "InfAmmo: EnableHook failed with error: " << MH_StatusToString(status) << std::endl;
                    shutdown(fp, "InfAmmo: EnableHook failed!");
                    return 1;
                }
            }
            else {
                std::cout << "Infinite Ammo Disabled" << std::endl;
                status = MH_DisableHook(reinterpret_cast<void**>(FuncTarget));
                if (status != MH_OK && status != MH_ERROR_DISABLED) { 
                    std::cout << "InfAmmo: DisableHook failed with error: " << MH_StatusToString(status) << std::endl;
                    shutdown(fp, "InfAmmo: DisableHook failed!");
                    return 1;
                }
            }
        }
        Sleep(100);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Menue, hModule, 0, 0);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
