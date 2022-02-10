project(testblur)
set(SRCS
	TestBlur.h TestBlur.cpp
)
set(FILES
	testbloom/bloom_scene.png
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES testcore)
