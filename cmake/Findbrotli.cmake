# Prefer system libbrotlidec (Debian/Fedora/Arch pkg-config name).
# Falls back to the bundled decoder in contrib/libs/brotli via engine_add_library.
if (DEFINED BROTLI_FOUND)
	return()
endif()
if (DEFINED BROTLI_LOCAL OR USE_LIBS_FORCE_LOCAL)
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
	/usr/local/opt/brotli
	$ENV{VCPKG_ROOT}/installed/${_PROCESSOR_ARCH}-windows
	C:/Tools/vcpkg/installed/${_PROCESSOR_ARCH}-windows
	C:/vcpkg/installed/${_PROCESSOR_ARCH}-windows
)
find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_BROTLI libbrotlidec)
endif()
find_path(BROTLI_INCLUDE_DIRS
	NAMES brotli/decode.h
	HINTS ENV BROTLIDIR
	PATH_SUFFIXES include
	PATHS
		${_BROTLI_INCLUDE_DIRS}
		${_SEARCH_PATHS}
)
find_library(BROTLI_DEC_LIBRARY
	NAMES brotlidec
	HINTS ENV BROTLIDIR
	PATH_SUFFIXES lib64 lib lib/${_PROCESSOR_ARCH}
	PATHS
		${_BROTLI_LIBRARY_DIRS}
		${_SEARCH_PATHS}
)
find_library(BROTLI_COMMON_LIBRARY
	NAMES brotlicommon
	HINTS ENV BROTLIDIR
	PATH_SUFFIXES lib64 lib lib/${_PROCESSOR_ARCH}
	PATHS
		${_BROTLI_LIBRARY_DIRS}
		${_SEARCH_PATHS}
)
set(BROTLI_LIBRARIES ${BROTLI_DEC_LIBRARY})
if (BROTLI_COMMON_LIBRARY)
	list(APPEND BROTLI_LIBRARIES ${BROTLI_COMMON_LIBRARY})
endif()
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(brotli FOUND_VAR BROTLI_FOUND REQUIRED_VARS BROTLI_INCLUDE_DIRS BROTLI_DEC_LIBRARY)
var_global(BROTLI_INCLUDE_DIRS BROTLI_LIBRARIES BROTLI_FOUND)
unset(_SEARCH_PATHS)
unset(_PROCESSOR_ARCH)
