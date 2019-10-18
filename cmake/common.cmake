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

if (USE_DOXYGEN_CHECK)
	check_cxx_compiler_flag("-Wdocumentation" COMPILER_SUPPORTS_WDOCUMENTATION)
	if (COMPILER_SUPPORTS_WDOCUMENTATION)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wdocumentation")
	endif()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FORTIFY_SOURCE=2 -D__STDC_FORMAT_MACROS")

check_c_compiler_flag("-fexpensive-optimizations" HAVE_EXPENSIVE_OPTIMIZATIONS)
unset(CMAKE_REQUIRED_FLAGS)

endif(NOT MSVC)
