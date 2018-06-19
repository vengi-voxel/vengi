project(testvoxelfont)
set(SRCS
	TestVoxelFont.h TestVoxelFont.cpp
)
set(FILES
	shared/font.ttf
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES testcore voxelfont voxelrender)
