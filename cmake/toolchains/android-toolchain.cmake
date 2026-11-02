# Wrapper around the official Android NDK CMake toolchain.
#
# Required:
#   ANDROID_NDK_HOME or ANDROID_NDK
#
# Optional:
#   ANDROID_ABI       (default: arm64-v8a)
#   ANDROID_PLATFORM  (default: android-24)
#   ANDROID_STL       (default: c++_static)

if (DEFINED ENV{ANDROID_NDK_HOME} AND NOT "$ENV{ANDROID_NDK_HOME}" STREQUAL "")
	set(ANDROID_NDK "$ENV{ANDROID_NDK_HOME}")
elseif (DEFINED ENV{ANDROID_NDK} AND NOT "$ENV{ANDROID_NDK}" STREQUAL "")
	set(ANDROID_NDK "$ENV{ANDROID_NDK}")
elseif (NOT ANDROID_NDK)
	message(FATAL_ERROR "ANDROID_NDK_HOME (or ANDROID_NDK) is not set")
endif()

if (NOT ANDROID_PLATFORM)
	set(ANDROID_PLATFORM android-24)
endif()
if (NOT ANDROID_ABI)
	set(ANDROID_ABI arm64-v8a)
endif()
if (NOT ANDROID_STL)
	set(ANDROID_STL c++_static)
endif()

set(_VENGI_NDK_TOOLCHAIN "${ANDROID_NDK}/build/cmake/android.toolchain.cmake")
if (NOT EXISTS "${_VENGI_NDK_TOOLCHAIN}")
	message(FATAL_ERROR "NDK CMake toolchain not found: ${_VENGI_NDK_TOOLCHAIN}")
endif()
include("${_VENGI_NDK_TOOLCHAIN}")
