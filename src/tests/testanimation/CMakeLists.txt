project(testanimation)
set(SRCS
	TestAnimation.h TestAnimation.cpp
)
set(FILES
	testanimation/testanimation-keybindings.cfg
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES testcore animation)
