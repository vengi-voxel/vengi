set(SRCS
	cl/CLComputeVideo.cpp

	ComputeVideo.h
	ComputeVideo.cpp
)
set(LIB computevideo)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES compute video)
