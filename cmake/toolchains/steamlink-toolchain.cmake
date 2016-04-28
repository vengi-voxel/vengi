# see https://cmake.org/Wiki/CMake_Useful_Variables
# see https://cmake.org/Wiki/CMake_Cross_Compiling

if ("$ENV{MARVELL_SDK_PATH}" STREQUAL "")
	message(FATAL_ERROR "You need to export the sdk path of your steamlink directory to the env var MARVELL_SDK_PATH")
endif()
set(MARVELL_SDK_PATH $ENV{MARVELL_SDK_PATH})

set(CMAKE_SYSROOT ${MARVELL_SDK_PATH}/rootfs)
set(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_CROSS_COMPILING ON) # Workaround for http://www.cmake.org/Bug/view.php?id=14075
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)
set(STEAMLINK 1)
set(CMAKE_PREFIX_PATH ${MARVELL_SDK_PATH}/toolchain/bin)
set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_STRIP armv7a-cros-linux-gnueabi-strip)
set(CMAKE_AS armv7a-cros-linux-gnueabi-as)
set(CMAKE_C_COMPILER_INIT armv7a-cros-linux-gnueabi-gcc)
set(CMAKE_CXX_COMPILER_INIT armv7a-cros-linux-gnueabi-g++)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
set(CMAKE_C_FLAGS_INIT "--sysroot=${CMAKE_SYSROOT} -marm -mfloat-abi=hard")
set(CMAKE_CXX_FLAGS_INIT "--sysroot=${CMAKE_SYSROOT} -marm -mfloat-abi=hard")
set(ENV{PKG_CONFIG_PATH} "")
set(ENV{PKG_CONFIG_LIBDIR} "${CMAKE_SYSROOT}/usr/lib/pkgconfig")
add_definitions(-DSTEAMLINK)

