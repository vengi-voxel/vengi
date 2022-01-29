project(testbloom)
set(SRCS
	TestBloom.h TestBloom.cpp
)
set(FILES
	testbloom/bloom_scene.png
	testbloom/bloom_extracted.png
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES testcore)
