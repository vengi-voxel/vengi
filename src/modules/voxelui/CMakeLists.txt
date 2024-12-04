set(LIB voxelui)
set(SRCS
	FileDialogOptions.h FileDialogOptions.cpp
	LUAApiWidget.h LUAApiWidget.cpp
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxelformat ui)
