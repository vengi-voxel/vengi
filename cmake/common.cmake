include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)
include(CheckCCompilerFlag)

set(DEFAULT_LUA_EXECUTABLE lua lua5.2 lua5.3)
set(DEFAULT_LUAC_EXECUTABLE luac luac5.2 luac5.3)
set(DATA_DIR ${ROOT_DIR}/data CACHE STRING "" FORCE)
set(NATIVE_BUILD_DIR ${ROOT_DIR}/build CACHE PATH "The directory where the initial native tool were built")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Compile Type" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel RelWithDebInfo Profile)

if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	set(RELEASE False)
else()
	set(RELEASE True)
endif()

if (${CMAKE_BUILD_TYPE} MATCHES "Release")
set(CMAKE_LINK_WHAT_YOU_USE ON)
endif()

if (MSVC)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP /TC /errorReport:queue /DWIN32 /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS /wd4244 /wd4100 /wd4267")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Zi /Od /Oy- /MTd /D_DEBUG /DDEBUG=1")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Ox /MT /DNDEBUG")

# 4456: declaration of 'q' hides previous local declaration
# 4244: possible loss of data (e.g. float/double)
# 4201: nonstandard extension used
# 4245: signed/unsigned mismatch
# 4100: unreferenced formal parameter
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /MP /TP /DWIN32 /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS /wd4244 /wd4245 /wd4201 /wd4100 /wd4456 /wd4267")
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
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT HAVE_LTO OUTPUT error)
if (HAVE_LTO)
	if (${CMAKE_BUILD_TYPE} MATCHES "Release")
		set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
		message(STATUS "IPO / LTO enabled")
	else()
		message(STATUS "IPO / LTO only enabled in release builds")
	endif()
else()
	message(STATUS "IPO / LTO not supported: <${error}>")
endif()

if (NOT MSVC)

find_program(DESKTOP_FILE_VALIDATE_EXECUTABLE desktop-file-validate)

if (USE_GPROF)
	check_cxx_compiler_flag("-pg" COMPILER_SUPPORTS_GNUPROF)
	if (COMPILER_SUPPORTS_GNUPROF)
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg")
	endif()
endif()

check_cxx_compiler_flag("-Wthread-safety" HAVE_THREAD_SAFETY_CHECKS)
if (HAVE_THREAD_SAFETY_CHECKS)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wthread-safety")
endif()
check_cxx_compiler_flag("-fdiagnostics-parseable-fixits" CXX_COMPILER_SUPPORTS_FIXITS)
if (CXX_COMPILER_SUPPORTS_FIXITS)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-parseable-fixits")
endif()
check_c_compiler_flag("-fdiagnostics-parseable-fixits" C_COMPILER_SUPPORTS_FIXITS)
if (C_COMPILER_SUPPORTS_FIXITS)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdiagnostics-parseable-fixits")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FORTIFY_SOURCE=2 -D__STDC_FORMAT_MACROS")

check_c_compiler_flag("-fexpensive-optimizations" HAVE_EXPENSIVE_OPTIMIZATIONS)
unset(CMAKE_REQUIRED_FLAGS)

endif(NOT MSVC)
