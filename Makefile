# Convenience wrapper for cross-compiling from Linux/macOS with MinGW-w64.
#
#   make          build both architectures into build/
#   make x64      64-bit only
#   make x86      32-bit only
#   make clean
#
# On Windows, use the Visual Studio solution or CMake instead -- see README.md.

SRC := DLL-Template/Dll-Template.cpp
OUT := build

CXXFLAGS := -O2 -Wall -Wextra -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN
# -s strips symbols; the static libgcc/libstdc++ links mean the DLL has no
# sidecar runtime dependencies.
LDFLAGS := -shared -static-libgcc -static-libstdc++ -s -Wl,--subsystem,windows

CXX_X64 := x86_64-w64-mingw32-g++
CXX_X86 := i686-w64-mingw32-g++

.PHONY: all x64 x86 clean

all: x64 x86

x64: $(OUT)/x64/Dll-Template.dll
x86: $(OUT)/x86/Dll-Template.dll

$(OUT)/x64/Dll-Template.dll: $(SRC)
	@mkdir -p $(@D)
	$(CXX_X64) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

$(OUT)/x86/Dll-Template.dll: $(SRC)
	@mkdir -p $(@D)
	$(CXX_X86) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -rf $(OUT)
