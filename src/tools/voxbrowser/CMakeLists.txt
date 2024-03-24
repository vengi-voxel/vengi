include_directories(modules)
add_subdirectory(modules)

project(voxbrowser)
set(SRCS
	VoxBrowser.h VoxBrowser.cpp
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED DESCRIPTION "Voxel Browser")
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES voxbrowser-ui voxelcollection)
