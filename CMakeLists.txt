cmake_minimum_required(VERSION 3.15)

project(DllTemplate LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR
        "This project targets Windows. To cross-compile from Linux, pass a "
        "toolchain file, e.g. -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake")
endif()

add_library(Dll-Template SHARED
    DLL-Template/Dll-Template.cpp
)

set_target_properties(Dll-Template PROPERTIES
    # Without this, MinGW emits "libDll-Template.dll".
    PREFIX ""
    OUTPUT_NAME "Dll-Template"
)

target_compile_definitions(Dll-Template PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
)

if(MSVC)
    # Static CRT: the DLL then has no dependency on the Visual C++
    # redistributable being installed wherever it ends up.
    set_property(TARGET Dll-Template PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_options(Dll-Template PRIVATE /W4 /permissive-)
else()
    # Same reasoning for MinGW: no libgcc/libstdc++ DLLs to ship alongside.
    target_compile_options(Dll-Template PRIVATE -Wall -Wextra)
    target_link_options(Dll-Template PRIVATE
        -static-libgcc -static-libstdc++
        # Match the MSVC build, which sets SubSystem=Windows.
        -Wl,--subsystem,windows)
endif()
