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

`Dll-Template.cpp` is the only source file.

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

## Extending the template

### Adding source files

There are two build definitions and **both** need the new file, or the MSVC and
MinGW builds drift apart and CI catches it as a link error:

1. `CMakeLists.txt` — add it to the `add_library(Dll-Template SHARED ...)` list.
2. `DLL-Template/Dll-Template.vcxproj` — add a `<ClCompile Include="yours.cpp" />`
   to the existing `ItemGroup`. Visual Studio does this for you when you add the
   file through the IDE.

Headers need no entry in either. The `Makefile` compiles `$(SRC)` only, so add
the file to that variable too if you use the Makefile route.

There is no precompiled header, so nothing has to be included first. Just
`#include <windows.h>` where you need it.

### Adding exports

The simplest route is what `test()` already does:

```cpp
extern "C" __declspec(dllexport) BOOL myFunction(void)
```

`extern "C"` suppresses C++ name mangling, and for a `__cdecl` function this
exports the name undecorated on both x86 and x64 — so `GetProcAddress(h,
"myFunction")` works on both. Drop the `extern "C"` and you get
`?myFunction@@YAHXZ` instead.

The exception is `__stdcall`/`WINAPI` functions on x86, which decorate to
`_myFunction@N` regardless. If you need one of those exported under a clean
name, add a module definition file:

```
EXPORTS
    myFunction
```

Reference it from the `.vcxproj` with `<ModuleDefinitionFile>`, and pass it to
MinGW by listing the `.def` on the link line. CI asserts that `test()` stays
undecorated, which is the kind of regression a `.def` change can introduce
silently.

### Where to start

- Replace the two `MessageBoxW` calls — they are placeholders.
- Read **Working inside DllMain** below before adding anything to `DllMain`.
  The constraints there are the single biggest source of DLLs that work on one
  machine and hang on another.

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

## Verifying it loaded

In a lab you usually want to answer two questions: did it load at all, and
which process picked it up.

**Did it load.** The `DllMain` message box is the signal, with one significant
caveat: a message box needs an interactive desktop. A DLL loaded by a service,
a scheduled task, or anything else in Session 0 will call `MessageBoxW`, get no
visible window, and block on it — which looks exactly like "nothing happened"
while actually wedging the host. If the host might not be interactive, replace
the message box with something you can observe out of band (a file write, an
event log entry, a named pipe) before you go looking for bugs.

**Which process loaded it.** From the target:

```bat
tasklist /m Dll-Template.dll
```

That lists every process with the module mapped. Sysinternals `listdlls.exe
-d Dll-Template.dll` gives the same answer with the full path it was loaded
from, which matters when more than one copy exists on disk.

**Watching load attempts.** Process Monitor with a path filter on the DLL name
shows each attempt and its result, so you can see a load being attempted and
failing, as opposed to never being attempted at all. The two are easy to
confuse and lead to very different debugging.

## When nothing happens

In rough order of how often it is the answer:

1. **Architecture mismatch.** A 32-bit process cannot load a 64-bit DLL or vice
   versa. The load fails with `ERROR_BAD_EXE_FORMAT` (193). Check it first:

   ```sh
   file build/x64/Dll-Template.dll    # PE32+ executable (DLL) (GUI) x86-64
   ```

   On Windows, `dumpbin /headers Dll-Template.dll` reports the machine type.

2. **The load was never attempted.** Confirm with Process Monitor rather than
   inferring it from the absence of a message box.

3. **A dependency is missing.** As shipped, the DLL imports only `kernel32`,
   `user32`, and — on MinGW builds — `msvcrt.dll`, all of which are present on
   every Windows install, so this should not bite until you add a library.
   `dumpbin /dependents` or MinGW's `objdump -p | grep 'DLL Name'` lists what
   the DLL needs.

4. **Session 0.** See the caveat above — a non-interactive host means the
   message box is invisible, not absent.

5. **`DllMain` returned `FALSE`.** The loader treats that as initialisation
   failure and unloads the DLL. If you add a failure path, make sure it is
   actually meant to abort the load.

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

## Intended use

This is a template for building DLLs — development, debugging, and
authorised security testing such as your own labs, CTFs, and engagements you
have written permission for. Test it only against systems you own or are
authorised to assess.

## License

MIT — see [LICENSE](LICENSE).
