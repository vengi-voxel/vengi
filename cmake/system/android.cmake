include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

# Desktop OpenGL and libunwind stack traces are not available on Android.
set(USE_OPENGLES ON CACHE BOOL "Enable OpenGLES" FORCE)
set(USE_STACKTRACES OFF CACHE BOOL "Enable stacktraces" FORCE)
set(USE_SANITIZERS OFF CACHE BOOL "Enable sanitizer" FORCE)

if (NOT CMAKE_ANDROID_NDK AND DEFINED ENV{ANDROID_NDK_HOME})
	set(CMAKE_ANDROID_NDK "$ENV{ANDROID_NDK_HOME}")
endif()
if (NOT CMAKE_ANDROID_NDK AND DEFINED ENV{ANDROID_NDK})
	set(CMAKE_ANDROID_NDK "$ENV{ANDROID_NDK}")
endif()

message(STATUS "Android NDK root: ${CMAKE_ANDROID_NDK}")
message(STATUS "Android ABI: ${CMAKE_ANDROID_ARCH_ABI}")
message(STATUS "Android platform: ${CMAKE_SYSTEM_VERSION}")

# Match the desktop build: the engine does not use exceptions or RTTI.
# The NDK promotes -Wformat-security to an error. Desktop builds do not.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-error=format-security -D_GNU_SOURCE -D_DEFAULT_SOURCE")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti -Wall -Wextra -Wno-unused-parameter -Wno-error=format-security -D_GNU_SOURCE -D_DEFAULT_SOURCE -D__STDC_FORMAT_MACROS")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# 64-bit atomics on 32-bit ABIs, and the Android log library used by the engine.
link_libraries(atomic log)

if (USE_CCACHE)
	find_program(CCACHE "ccache")
	if (CCACHE)
		message(STATUS "Using ccache")
		set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE})
		set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
	endif()
endif()
