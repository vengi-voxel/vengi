project(testturbobadger)
set(SRCS
	Demo01.cpp Demo.h
	ListWindow.cpp ListWindow.h
	ResourceEditWindow.cpp ResourceEditWindow.h
	TestTurbobadger.h TestTurbobadger.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES turbobadger)
