set(SRCS
	ClientPager.h
	ClientPager.cpp
)
set(LIB client-voxel)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES http voxelworld)
target_include_directories(${LIB} PUBLIC ..)
