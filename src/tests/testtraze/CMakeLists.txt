find_package(Mosquitto)

if (MOSQUITTO_FOUND)
	project(testtraze)
	set(SRCS
		TrazeEvents.h
		TrazeProtocol.h TrazeProtocol.cpp
		TrazeTypes.h
		TestTraze.h TestTraze.cpp
	)
	set(FILES
		shared/font.ttf
		testtraze/testtraze-keybindings.cfg
	)
	engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED NOINSTALL)
	engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES testcore voxelrender voxelfont ${MOSQUITTO_LIBRARIES})
else()
	message(WARNING "${PROJECT_NAME} is disabled - libmosquitto wasn't found")
endif()
