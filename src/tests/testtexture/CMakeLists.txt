project(testtexture)
set(SRCS
	TestTexture.h TestTexture.cpp
)
set(FILES
	testtexture/texture.png
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES testcore)
