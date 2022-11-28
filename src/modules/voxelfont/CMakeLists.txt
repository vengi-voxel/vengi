set(LIB voxelfont)
set(SRCS
	external/stb_truetype.h
	VoxelFont.h VoxelFont.cpp
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxel)
