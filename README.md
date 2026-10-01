# DLL-Template

A minimal, correct skeleton for a Windows C++ DLL.

Out of the box it shows a message box from `DllMain` when the DLL is loaded, and
a second one from an exported `test()` function. That is all it does — it is a
starting point for your own code, and a quick way to confirm that a given
process really is loading the DLL you think it is.

It builds with Visual Studio on Windows, and cross-compiles from Linux or macOS
with MinGW-w64.

## Layout

```
CMakeLists.txt                     portable build (MSVC and MinGW)
Makefile                           one-command MinGW cross-compile
cmake/mingw-w64-x86_64.cmake       CMake cross-compile toolchain, 64-bit
cmake/mingw-w64-i686.cmake         CMake cross-compile toolchain, 32-bit
DLL-Template/
  Dll-Template.sln                 Visual Studio solution
  Dll-Template.vcxproj             Visual Studio project
  Dll-Template.cpp                 DllMain + the exported test() function
```

`Dll-Template.cpp` is the only source file. Add yours next to it and list them
in both `CMakeLists.txt` and the `.vcxproj`.

## Building

### Visual Studio (Windows)

Open `DLL-Template/Dll-Template.sln` and build. Visual Studio 2019 or 2022.

The project no longer pins a toolset or an SDK version: it asks for
`$(DefaultPlatformToolset)` and Windows SDK `10.0`, which resolve to whatever
that machine has installed. You should not see a retarget prompt.

Or from a Developer Command Prompt:

```bat
msbuild DLL-Template\Dll-Template.vcxproj /p:Configuration=Release /p:Platform=x64
```

### CMake (Windows)

```bat
cmake -B build -A x64
cmake --build build --config Release
```

### MinGW-w64 (cross-compile from Linux or macOS)

```sh
sudo apt-get install mingw-w64     # Debian/Ubuntu
make                               # both architectures -> build/x64, build/x86
```

Or through CMake, if you want the same build system everywhere:

```sh
cmake -B build/x64 -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build/x64
```

All build paths link the C runtime statically, so the resulting DLL does not
need the Visual C++ redistributable (MSVC) or `libstdc++-6.dll` and
`libgcc_s_*.dll` (MinGW) present on the machine that loads it.

## Loading it

```bat
rundll32 Dll-Template.dll test
```

`rundll32` is the quickest check, though note it expects the
`void CALLBACK (HWND, HINSTANCE, LPSTR, int)` signature for anything more than
a smoke test — `test()` returns `BOOL` and takes no arguments, so it works here
only because nothing inspects the return value.

From your own code, the normal route:

```c
HMODULE h = LoadLibraryW(L"Dll-Template.dll");   // fires DllMain
BOOL (*fn)(void) = (BOOL(*)(void))GetProcAddress(h, "test");
fn();
```

`test()` is exported undecorated on both x86 and x64 — verified in CI — so
`GetProcAddress(h, "test")` works without the usual `_test@0` guesswork.

## Working inside DllMain

`DllMain` runs while the loader lock is held, and that makes most of the Win32
API off-limits. Specifically, do not call from `DllMain`:

- `LoadLibrary` / `GetModuleHandle` on a module that is not already loaded —
  this is a straight deadlock against the lock you are already holding
- anything in COM, winsock, or the CRT's startup-dependent machinery
- anything in `user32`, `shell32`, `ole32`, or any DLL whose own initialiser
  may not have run yet — load order is not guaranteed
- any wait on another thread, which cannot make progress if it needs the loader

The template's own `MessageBoxW` call is a `user32` call and therefore already
over that line. It is left in because it is the shortest possible "did this
load?" signal and the risk is acceptable for that, but it is not a pattern to
extend. Microsoft's guidance is
[Dynamic-Link Library Best Practices](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices).

The real-world consequence of ignoring this: the DLL works on your bench and
deadlocks on a different machine, or in a different host process, because load
order and timing changed. If you need to do real work on load, have `DllMain`
start a thread and return immediately, and do the work there.

`DisableThreadLibraryCalls` is called on process attach, since the template has
no use for the per-thread notifications. Remove that call if you add
`DLL_THREAD_ATTACH` or `DLL_THREAD_DETACH` handling.

## Architecture has to match

A 32-bit process cannot load a 64-bit DLL or vice versa; the load fails with
`ERROR_BAD_EXE_FORMAT` (193). This is the most common reason a DLL that is
sitting in exactly the right place still does not load, so check it first:

```sh
file build/x64/Dll-Template.dll    # PE32+ executable (DLL) (GUI) x86-64
```

## Intended use

This is a template for building DLLs — development, debugging, and
authorised security testing such as your own labs, CTFs, and engagements you
have written permission for. Test it only against systems you own or are
authorised to assess.

## License

See the repository for license terms.
