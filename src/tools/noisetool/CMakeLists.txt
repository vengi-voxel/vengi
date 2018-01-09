project(noisetool)
set(SRCS
	NoiseTool.h NoiseTool.cpp
	NoiseData.h
	NoiseType.h NoiseType.cpp

	ui/NoiseToolWindow.h ui/NoiseToolWindow.cpp
	ui/NoiseDataNodeWindow.h ui/NoiseDataNodeWindow.cpp

	ui/noisedata/NoiseDataNodeWidget.h ui/noisedata/NoiseDataNodeWidget.cpp
	ui/noisedata/NoiseDataItemWidget.h ui/noisedata/NoiseDataItemWidget.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES turbobadger frontend util collection)
check_ui_turbobadger(${PROJECT_NAME})
