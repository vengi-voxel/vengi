set(CMAKE_SYSTEM_NAME Windows)

set(CMAKE_C_STANDARD_LIBRARIES "kernel32.lib user32.lib gdi32.lib winspool.lib winmm.lib imm32.lib comctl32.lib version.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib winhttp.lib dbghelp.lib wsock32.lib ws2_32.lib iphlpapi.lib rpcrt4.lib wininet.lib")
set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES}")
# /W4: set warning level to 4
# /MP: build with multiple processes
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /W4")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP /W4")
if (USE_SANITIZERS)
	# /fsanitize=address: enable address sanitizer
	# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
	# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fsanitize=address")
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# /INCREMENTAL:NO: disable incremental linking
# /DEBUG: generate debug info (event in release builds)
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /INCREMENTAL:NO /DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL /Gy")

# /wd4100: unreferenced formal parameter
# /wd4201: nonstandard extension used
# /wd4244: possible loss of data (e.g. float/double)
# /wd4245: signed/unsigned mismatch
# /wd4267: conversion from 'size_t' to 'int', possible loss of data
# /wd4324: structure was padded due to alignment specifier
# /wd4456: declaration of 'q' hides previous local declaration
set(MSVC_DISABLED_WARNINGS "/wd4100 /wd4201 /wd4244 /wd4245 /wd4267 /wd4324 /wd4456")

# /Zi: generate PDB file
# /Od: disable optimizations
# /Oy-: disable Omit Frame Pointers
# /Ox: maximum optimizations
# /DNOMINMAX: don't define min/max macros
# /D_CRT_SECURE_NO_WARNINGS: don't warn about unsafe functions
# /errorReport:queue: queue error reports
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /errorReport:queue /DWIN32 /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS ${MSVC_DISABLED_WARNINGS}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Zi /Od /Oy- /D_DEBUG /DDEBUG=1")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2 /DNDEBUG")
# /EHsc: enable C++ exceptions
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /DWIN32 /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS /D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS ${MSVC_DISABLED_WARNINGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od /Oy- /D_DEBUG /DDEBUG=1")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")

# Visual Studio 2018 15.8 implemented conformant support for std::aligned_storage, but the conformant
# support is only enabled when the following flag is passed, to avoid
# breaking backwards compatibility with code that relied on the non-conformant behavior
# /D_ENABLE_EXTENDED_ALIGNED_STORAGE: enable conformant std::aligned_storage
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_ENABLE_EXTENDED_ALIGNED_STORAGE")

# /MANIFEST:NO: don't generate a manifest file
# /STACK:5000000: set the stack size to 5MB
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO /STACK:5000000")
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	# /machine:x64: set the target architecture to x64
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /machine:x64")
else()
	# /machine:x86: set the target architecture to x86
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /machine:x86")
endif()

if (USE_CCACHE)
	find_program(SCCACHE sccache)
	if (SCCACHE)
		message(STATUS "Using sccache")
		set(CMAKE_C_COMPILER_LAUNCHER ${SCCACHE})
		set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE})
		# /Z7: embed debug info in the PDB file
		if (CMAKE_BUILD_TYPE STREQUAL "Debug")
			string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
			string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
		elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
			string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
			string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
		elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
			string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
			string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
		endif()
	endif()
endif()
