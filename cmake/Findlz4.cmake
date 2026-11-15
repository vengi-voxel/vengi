# Prefer system liblz4 (pkg-config name on Debian/Fedora/Arch).
# Falls back to the bundled codec in contrib/libs/lz4 via engine_add_library.
if (DEFINED LZ4_FOUND AND LZ4_LIBRARIES)
	return()
endif()
if (DEFINED LZ4_LOCAL OR USE_LIBS_FORCE_LOCAL)
	return()
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_PROCESSOR_ARCH "x64")
else()
	set(_PROCESSOR_ARCH "x86")
endif()
set(_SEARCH_PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw
	/opt/local
	/opt/csw
	/opt
	/usr/local/opt
	/usr/local/opt/lz4
	$ENV{VCPKG_ROOT}/installed/${_PROCESSOR_ARCH}-windows
	C:/Tools/vcpkg/installed/${_PROCESSOR_ARCH}-windows
	C:/vcpkg/installed/${_PROCESSOR_ARCH}-windows
)
find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_LZ4 liblz4)
endif()
find_path(LZ4_INCLUDE_DIRS
	NAMES lz4.h lz4frame.h
	HINTS ENV LZ4DIR
	PATH_SUFFIXES include
	PATHS
		${_LZ4_INCLUDE_DIRS}
		${_SEARCH_PATHS}
)
find_library(LZ4_LIBRARIES
	NAMES lz4 liblz4
	HINTS ENV LZ4DIR
	PATH_SUFFIXES lib64 lib lib/${_PROCESSOR_ARCH}
	PATHS
		${_LZ4_LIBRARY_DIRS}
		${_SEARCH_PATHS}
)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(lz4 FOUND_VAR LZ4_FOUND REQUIRED_VARS LZ4_INCLUDE_DIRS LZ4_LIBRARIES)
var_global(LZ4_INCLUDE_DIRS LZ4_LIBRARIES LZ4_FOUND)
unset(_SEARCH_PATHS)
unset(_PROCESSOR_ARCH)
