add_subdirectory(binary_to_compressed_c)
find_program(GLSLANG_EXECUTABLE NAMES glslangValidator)
if (NOT GLSLANG_EXECUTABLE)
	macro(find_host_package)
		find_package(${ARGN})
	endmacro()
	if (MSVC)
		# 4389: signed/unsigned mismatch
		# 4457: declaration hides function parameter
		# 4458: declaration hides class member
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4389 /wd4457 /wd4458")
	endif()
	add_subdirectory(glslang)
else()
	add_executable(glslangValidator IMPORTED GLOBAL)
	set_property(TARGET glslangValidator PROPERTY IMPORTED_LOCATION ${GLSLANG_EXECUTABLE})
endif()
