set(SRCS
	src/gtest-all.cc
	src/gmock-all.cc
)
set(DEFINES GTEST_HAS_EXCEPTIONS=0 GTEST_HAS_RTTI=0)

engine_add_library(
	LIB gtest
	SRCS ${SRCS}
	DEFINES ${DEFINES}
)
if (NOT GTEST_FOUND)
	target_include_directories(gtest PUBLIC . include/)
	if (${CMAKE_VERSION} VERSION_LESS "3.16.0")
		message(STATUS "Precompiled headers for googletest disabled - needs at least cmake 3.16.0")
	else()
		target_precompile_headers(gtest PRIVATE include/gtest/gtest.h)
	endif()
endif()
