set(LIB voxedit-ui)

set(SRCS
	VoxEditWindow.h VoxEditWindow.cpp
	Viewport.h Viewport.cpp
)

engine_add_module(TARGET ${LIB} SRCS ${SRCS} FILES ${FILES} DEPENDENCIES imgui voxedit-util)
