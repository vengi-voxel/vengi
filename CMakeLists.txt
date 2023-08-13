cmake_minimum_required(VERSION 3.10)
include(CMakeLists.local OPTIONAL)

option(UNITTESTS "Builds with unittests" ON)
option(VISUALTESTS "Builds with visual tests (src/tests)" ON)

option(VOXEDIT "Builds voxedit (needs TOOLS to be active)" ON)
option(THUMBNAILER "Builds thumbnailer (needs TOOLS to be active)" ON)
option(VOXCONVERT "Builds voxconvert (needs TOOLS to be active)" ON)

option(USE_OPENGLES "Enable OpenGLES" OFF)
option(USE_CCACHE "Use ccache" ON)
option(USE_DOXYGEN_CHECK "Use -Wdocumentation if available" OFF)
option(USE_CPPCHECK "Enable cppcheck" OFF)
option(USE_CLANG_TIDY "Enable Clang Tidy" OFF)
option(USE_STACKTRACES "Enable stacktraces" ON)

set(PKGDATADIR "" CACHE STRING "System directory to search for data files (must end on /)")

set(ROOT_PROJECT_MAJOR_VERSION 0)
set(ROOT_PROJECT_MINOR_VERSION 0)
set(ROOT_PROJECT_MICRO_VERSION 27)
set(ROOT_PROJECT_PATCH_VERSION 0)
set(ROOT_PROJECT_VERSION "${ROOT_PROJECT_MAJOR_VERSION}.${ROOT_PROJECT_MINOR_VERSION}.${ROOT_PROJECT_MICRO_VERSION}.${ROOT_PROJECT_PATCH_VERSION}")
set(ROOT_PROJECT_NAME vengi)
project(${ROOT_PROJECT_NAME} VERSION ${ROOT_PROJECT_VERSION})
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "Root dir")
set(SCRIPTS_CMAKE_DIR ${PROJECT_SOURCE_DIR}/cmake)
set(GENERATE_DIR ${CMAKE_BINARY_DIR}/generated CACHE STRING "Generated code dir")
add_custom_target(codegen)

include(CMakeParseArguments)
include(CheckCXXCompilerFlag)
include(CheckCCompilerFlag)
include(CheckIncludeFiles)
include(GNUInstallDirs)

if (CMAKE_COMPILER_IS_GNUCC)
	set(USE_GCC TRUE)
elseif (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "AppleClang" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
	set(USE_CLANG TRUE)
elseif (MSVC)
	set(USE_MSVC TRUE)
endif()

if (APPLE)
	enable_language(OBJC)
	enable_language(OBJCXX)
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	set(WINDOWS 1)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
	set(LINUX 1)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	set(DARWIN 1)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set(LINUX 1)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
	set(EMSCRIPTEN 1)
	set(USE_STACKTRACES OFF)
	set(USE_OPENGLES ON)
else()
	message(WARNING "Unknown host system: ${CMAKE_SYSTEM_NAME}. Default to linux")
	set(LINUX 1)
endif()

include(${SCRIPTS_CMAKE_DIR}/common.cmake)
include(${SCRIPTS_CMAKE_DIR}/util.cmake)
include(${SCRIPTS_CMAKE_DIR}/genshader.cmake)
include(${SCRIPTS_CMAKE_DIR}/externallib.cmake)
include(${SCRIPTS_CMAKE_DIR}/valgrind.cmake)
include(${SCRIPTS_CMAKE_DIR}/perf.cmake)
include(${SCRIPTS_CMAKE_DIR}/heaptrack.cmake)
include(${SCRIPTS_CMAKE_DIR}/apitrace.cmake)
include(${SCRIPTS_CMAKE_DIR}/debug.cmake)
include(${SCRIPTS_CMAKE_DIR}/googletest.cmake)
include(${SCRIPTS_CMAKE_DIR}/lua.cmake)
include(${SCRIPTS_CMAKE_DIR}/macros.cmake)
include(${SCRIPTS_CMAKE_DIR}/pack.cmake)
include(CPack)

if (NOT CMAKE_TOOLCHAIN_FILE)
	if (MSVC)
		include(${SCRIPTS_CMAKE_DIR}/toolchains/msvc-toolchain.cmake)
	elseif (WINDOWS)
		include(${SCRIPTS_CMAKE_DIR}/toolchains/windows-toolchain.cmake)
	elseif (DARWIN)
		include(${SCRIPTS_CMAKE_DIR}/toolchains/darwin-toolchain.cmake)
	else()
		include(${SCRIPTS_CMAKE_DIR}/toolchains/linux-toolchain.cmake)
	endif()
else()
	if (MSVC)
		# 4456: declaration of 'q' hides previous local declaration
		# 4244: possible loss of data (e.g. float/double)
		# 4201: nonstandard extension used
		# 4245: signed/unsigned mismatch
		# 4100: unreferenced formal parameter
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT /wd4244 /wd4245 /wd4201 /wd4100 /wd4456 /wd4267")
	endif()
endif()

if (MSVC)
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
endif()

# First for the generic no-config case (e.g. with mingw)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib)

# Second, for multi-config builds (e.g. msvc)
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
	string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR})
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib)
endforeach()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${SCRIPTS_CMAKE_DIR})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if (USE_CCACHE)
	find_host_program(CCACHE "ccache")
	if (CCACHE)
		message(STATUS "Using ccache")
		set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE})
		set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
		#set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE})
		#set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE})
		if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
			set(ENV{CCACHE_CPP2} "yes")
			add_compile_options(-Qunused-arguments)
		endif()
	else()
		message(STATUS "Could not find ccache")
		find_host_program(SCCACHE sccache)
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
		else()
			message(STATUS "Could not find sccache")
		endif()
	endif()
endif()

find_host_program(GDB_EXECUTABLE NAMES ${GDB_BINARY} gdb)
find_host_program(LLDB_EXECUTABLE NAMES ${LLDB_BINARY} lldb)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# These includes are needed to let the include for IMPLICIT_DEPENDS for shaders work
include_directories(src/modules/video/shaders)
include_directories(src/modules/compute/shaders)

include_directories(src/modules)

add_subdirectory(contrib/libs)
add_subdirectory(tools)

if (UNITTESTS)
	enable_testing()
	set(CTEST_TEST_TIMEOUT 1800)
endif()
add_subdirectory(src)

configure_file(src/engine-config.h.in engine-config.h @ONLY)
message(STATUS "Generate engine-config.h in ${CMAKE_CURRENT_BINARY_DIR}")

find_package(Doxygen)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile-mcss.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile-mcss @ONLY)
if (DOXYGEN_FOUND)
	add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Doxygen" VERBATIM
		DEPENDS codegen
	)
endif(DOXYGEN_FOUND)
