project(testbiomes)
set(SRCS
	TestBiomes.h TestBiomes.cpp
)

set(FILES
	shared/biomes.lua
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES testcore voxelworld)
#generate_shaders(${PROJECT_NAME} testbiomes)
