add_library(yocto STATIC
	yocto_math.h yocto_color.h yocto_geometry.h
	yocto_noise.h yocto_sampling.h yocto_shading.h
	yocto_bvh.h yocto_bvh.cpp
	yocto_shape.h yocto_shape.cpp
	yocto_image.h yocto_image.cpp
	yocto_scene.h yocto_scene.cpp
	yocto_trace.h yocto_trace.cpp
	yocto_parallel.h
)
target_include_directories(yocto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(yocto PROPERTIES UNITY_BUILD ON CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
target_link_libraries(yocto PRIVATE stb_image)

if(UNIX AND NOT APPLE)
	find_package(Threads REQUIRED)
	target_link_libraries(yocto PUBLIC Threads::Threads)
endif(UNIX AND NOT APPLE)

# warning flags
if(UNIX)
	target_compile_options(yocto PUBLIC -Wall -Wno-conversion -Wno-parentheses -Wno-sign-conversion -Wno-sign-compare -Wno-implicit-float-conversion -Wno-unused-variable)
endif()

if(MSVC)
	target_compile_options(yocto PUBLIC /D_CRT_SECURE_NO_WARNINGS)
	target_compile_options(yocto PUBLIC /EHsc)
endif(MSVC)
engine_target_optimize(yocto)
