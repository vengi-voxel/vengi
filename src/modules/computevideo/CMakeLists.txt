set(SRCS
	ComputeVideo.h
	ComputeVideo.cpp
)
find_package(OpenCL)
if (OpenCL_INCLUDE_DIRS)
	list(APPEND SRCS cl/CLComputeVideo.cpp)
else()
	list(APPEND SRCS dummy/DummyComputeVideo.cpp)
endif()

set(LIB computevideo)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES compute video)
if (OpenCL_INCLUDE_DIRS)
	target_include_directories(${LIB} PRIVATE ${OpenCL_INCLUDE_DIRS})
endif()
