set(SRCS
	stb_truetype.h
	VoxelFont.h VoxelFont.cpp
)
set(LIB voxelfont)
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES voxel)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
