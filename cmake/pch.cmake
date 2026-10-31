# Create donors in the project scope: contrib/libs changes the compiler flags.
if (CMAKE_DISABLE_PRECOMPILE_HEADERS)
	return()
endif()

# Shared REUSE_FROM donors are not used on MSVC. Visual Studio still links
# cmake_pch.obj from a path the donor does not emit; see engine_reuse_sdl3_pch.
if (MSVC)
	return()
endif()

# Apple consumers such as app compile AppDocument.m. REUSE_FROM needs cmake_pch.objc.h.
if (APPLE)
	enable_language(OBJC)
endif()

function(engine_add_pch target type)
	set(_source "${CMAKE_CURRENT_BINARY_DIR}/${target}.cpp")
	if (type STREQUAL "EXECUTABLE")
		file(GENERATE OUTPUT "${_source}" CONTENT "int main() { return 0; }\n")
		add_executable(${target} EXCLUDE_FROM_ALL "${_source}")
	else()
		file(GENERATE OUTPUT "${_source}" CONTENT "")
		add_library(${target} OBJECT EXCLUDE_FROM_ALL "${_source}")
	endif()
	# SDL consumers contain C as well as C++ sources; test suites are C++ only.
	if ("C" IN_LIST ARGN)
		set(_csource "${CMAKE_CURRENT_BINARY_DIR}/${target}.c")
		file(GENERATE OUTPUT "${_csource}" CONTENT "")
		target_sources(${target} PRIVATE "${_csource}")
		if (APPLE)
			set(_msource "${CMAKE_CURRENT_BINARY_DIR}/${target}.m")
			file(GENERATE OUTPUT "${_msource}" CONTENT "")
			target_sources(${target} PRIVATE "${_msource}")
		endif()
	endif()
	target_compile_definitions(${target} PRIVATE SDL_MAIN_HANDLED)
	set_target_properties(${target} PROPERTIES FOLDER pch)
endfunction()

engine_sdl3_pch_headers(SDL_PCH)
# Executables use PIE, libraries use PIC. Each group shares one PCH per language.
foreach(_target sdl3-pch sdl3-pch-executable)
	if (_target STREQUAL "sdl3-pch-executable")
		engine_add_pch(${_target} EXECUTABLE C)
	else()
		engine_add_pch(${_target} OBJECT C)
	endif()
	target_link_libraries(${_target} PRIVATE sdl3)
	target_precompile_headers(${_target} PRIVATE ${SDL_PCH})
endforeach()

if (UNITTESTS)
	engine_add_pch(gtest-pch EXECUTABLE)
	target_link_libraries(gtest-pch PRIVATE gtest sdl3)
	target_compile_options(gtest-pch PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wno-undef>)
	target_precompile_headers(gtest-pch PRIVATE <gtest/gtest.h> ${SDL_PCH})
endif()
