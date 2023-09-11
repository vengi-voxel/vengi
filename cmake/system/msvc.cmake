set(CMAKE_SYSTEM_NAME Windows)

set(CMAKE_C_STANDARD_LIBRARIES "kernel32.lib user32.lib gdi32.lib winspool.lib winmm.lib imm32.lib comctl32.lib version.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib dbghelp.lib wsock32.lib ws2_32.lib iphlpapi.lib rpcrt4.lib wininet.lib")
set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /W4")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP /W4")
if (USE_SANITIZERS)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fsanitize=address")
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /INCREMENTAL:NO")
endif()

set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG")
# include debug symbols in release builds
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /INCREMENTAL:NO")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP /errorReport:queue /DWIN32 /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS /wd4244 /wd4100 /wd4267")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Zi /Od /Oy- /MTd /D_DEBUG /DDEBUG=1")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Ox /MT /DNDEBUG")

# 4100: unreferenced formal parameter
# 4201: nonstandard extension used
# 4244: possible loss of data (e.g. float/double)
# 4245: signed/unsigned mismatch
# 4267: conversion from 'size_t' to 'int', possible loss of data
# 4324: structure was padded due to alignment specifier
# 4456: declaration of 'q' hides previous local declaration
set(MSVC_DISABLED_WARNINGS "/wd4100 /wd4201 /wd4244 /wd4245 /wd4267 /wd4324 /wd4456")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /MP /DWIN32 /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS /D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS ${MSVC_DISABLED_WARNINGS}")
# Visual Studio 2018 15.8 implemented conformant support for std::aligned_storage, but the conformant
# support is only enabled when the following flag is passed, to avoid
# breaking backwards compatibility with code that relied on the non-conformant behavior
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_ENABLE_EXTENDED_ALIGNED_STORAGE")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od /Oy- /MTd /D_DEBUG /DDEBUG=1")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ox /MT /DNDEBUG")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO /STACK:5000000")
if (CMAKE_CL_64)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /machine:x64")
else()
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /machine:x86")
endif()
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG")

if (USE_CCACHE)
	find_program(SCCACHE sccache)
	if (SCCACHE)
		message(STATUS "Using sccache")
		set(CMAKE_C_COMPILER_LAUNCHER ${SCCACHE})
		set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE})
		if (MSVC)
			if(CMAKE_BUILD_TYPE STREQUAL "Debug")
				string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
				string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
			elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
				string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
				string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
			elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
				string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
				string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
			endif()
		endif()
	endif()
endif()

foreach(flag_var
	CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
	CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
	CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
	CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
	if (${flag_var} MATCHES "/MD")
		string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
	endif()
endforeach()

foreach(flag_var
	CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
	CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
	CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
	CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
	string(REGEX REPLACE "/RTC(su|[1su])" "" ${flag_var} "${${flag_var}}")
endforeach(flag_var)
