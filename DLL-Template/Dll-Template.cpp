// Dll-Template.cpp : entry point for the DLL.
//
// A minimal, correct skeleton for a Windows C++ DLL. The demo behaviour -- a
// message box on load, and a second one from an exported function -- is
// deliberately trivial. It answers "did my DLL get loaded, and by what?" and
// nothing more. Replace it with whatever you are actually testing.
//
// Before you add anything to DllMain, read the "Working inside DllMain"
// section of README.md. The constraints there are real and the failures they
// cause are machine-dependent.

#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // We make no use of the per-thread notifications, so ask the loader to
        // stop delivering them: fewer callbacks, and no DllMain re-entry every
        // time the host process spins up a thread.
        DisableThreadLibraryCalls(hModule);

        // NOTE: this runs while the loader lock is held. A MessageBox here is
        // already more than the documented contract allows -- acceptable for a
        // throwaway load check, not a pattern to build on. See README.md.
        MessageBoxW(NULL, L"DllMain loaded", L"DLL-Template", MB_OK);
        break;

    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}

// Exported so the DLL produces an import library, and so it can be driven
// explicitly (GetProcAddress, or `rundll32 Dll-Template.dll test`).
extern "C" __declspec(dllexport) BOOL test(void)
{
    MessageBoxW(NULL, L"Exported test() function loaded", L"DLL-Template", MB_OK);

    return TRUE;
}
