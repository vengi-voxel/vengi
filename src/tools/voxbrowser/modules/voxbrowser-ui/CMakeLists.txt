set(LIB voxbrowser-ui)

set(SRCS
	MainWindow.h MainWindow.cpp
	StatusBar.h StatusBar.cpp
	MenuBar.h MenuBar.cpp
)

engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ui voxbrowser-util voxelrender)
