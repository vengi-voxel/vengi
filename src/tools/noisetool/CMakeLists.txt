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
target_link_libraries(${PROJECT_NAME} frontend util)
check_ui_files(${PROJECT_NAME})
