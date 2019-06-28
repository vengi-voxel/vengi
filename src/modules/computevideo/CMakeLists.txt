set(SRCS
	ComputeVideo.h
	ComputeVideo.cpp
)
find_package(OpenCL)
if (OpenCL_INCLUDE_DIRS)
	list(APPEND SRCS
		cl/CLComputeVideo.cpp
	)
else()
	# if opencl wasn't found, cuda might take over
	find_package(CUDA)
	if (CUDA_FOUND)
		list(APPEND SRCS
			cuda/CUDAComputeVideo.cpp
		)
		if (CUDA_TOOLKIT_INCLUDE)
			target_include_directories(${LIB} PRIVATE ${CUDA_TOOLKIT_INCLUDE})
		endif()
	endif()
endif()

if (NOT OpenCL_FOUND AND NOT OpenCL_INCLUDE_DIRS)
	list(APPEND SRCS
		dummy/DummyComputeVideo.cpp
	)
endif()

set(LIB computevideo)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES compute video)
if (OpenCL_INCLUDE_DIRS)
	target_include_directories(${LIB} PRIVATE ${OpenCL_INCLUDE_DIRS})
endif()
if (CUDA_TOOLKIT_INCLUDE)
	target_include_directories(${LIB} PRIVATE ${CUDA_TOOLKIT_INCLUDE})
endif()
