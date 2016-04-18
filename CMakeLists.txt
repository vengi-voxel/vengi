cmake_minimum_required(VERSION 2.8)
get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
get_filename_component(ENGINE_ROOT_DIR "." ABSOLUTE)
include("${FIPS_ROOT_DIR}/cmake/fips.cmake")
include("${ENGINE_ROOT_DIR}/cmake/common.cmake")
set(CMAKE_MODULE_PATH "${ENGINE_ROOT_DIR}/cmake")
include(CheckFunctionExists)
include(CheckLibraryExists)

set(FIPS_EXCEPTIONS ON)
set(FIPS_UNITTESTS ON)
set(FIPS_RTTI ON)
set(TB_ENABLE_RTTI ON CACHE BOOL "Do not disable RTTI" FORCE)

if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
elseif (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Darwin")
elseif (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
	# TODO: this is for the header only sauce lib
	add_definitions("-Wall -Wextra -Wno-deprecated-declarations -Wno-unused-local-typedefs -Wno-mismatched-tags")
endif()

include_directories(src/modules)
include_directories(src/modules/ui)

fips_setup(PROJECT engine)
gtest_suite_begin(tests TEMPLATE src/modules/core/tests/main.cpp.in)
fips_add_subdirectory(src)
gtest_suite_end(tests)
fips_finish()

copy_data_files(shared)