project(testimgui)
set(SRCS
	TestIMGUI.h TestIMGUI.cpp

	Demo.cpp
)
if (USE_IMPLOT_DEMO)
	list(APPEND SRCS implot_demo.cpp)
endif()
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES testcore ui)
