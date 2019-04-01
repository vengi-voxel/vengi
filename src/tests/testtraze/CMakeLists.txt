project(testtraze)
find_package(Mosquitto)

if (MOSQUITTO_FOUND)
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
	engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES testcore voxelrender voxelfont util audio ${MOSQUITTO_LIBRARIES})

	gtest_suite_begin(tests-${PROJECT_NAME} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
	gtest_suite_sources(tests-${PROJECT_NAME}
		tests/TrazeProtocolTest.cpp
		TrazeProtocol.cpp
		../../modules/core/tests/AbstractTest.cpp
	)
	gtest_suite_deps(tests-${PROJECT_NAME} core voxel ${MOSQUITTO_LIBRARIES})
	gtest_suite_end(tests-${PROJECT_NAME})
else()
	message(WARNING "${PROJECT_NAME} is disabled - libmosquitto wasn't found")
endif()
