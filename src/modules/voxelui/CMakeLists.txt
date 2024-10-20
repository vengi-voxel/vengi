set(LIB voxelui)
set(SRCS
	FileDialogOptions.h FileDialogOptions.cpp
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxelformat ui)
