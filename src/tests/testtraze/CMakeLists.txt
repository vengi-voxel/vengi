project(testtraze)
find_package(Mosquitto)

if (MOSQUITTO_FOUND)
	set(SRCS
		JSON.h json.hpp
		TrazeEvents.h
		TrazeProtocol.h TrazeProtocol.cpp
		TrazeTypes.h
		TestTraze.h TestTraze.cpp
	)
	set(FILES
		shared/font.ttf
		testtraze/testtraze-keybindings.cfg
		testtraze/sound/frag.ogg
		testtraze/sound/suicide.ogg
		testtraze/sound/collision.ogg
		testtraze/sound/join.ogg
		testtraze/sound/you_lose.ogg
		testtraze/sound/you_win.ogg
	)
	engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED NOINSTALL)
	engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES testcore uuidutil voxelrender voxelfont util audio ${MOSQUITTO_LIBRARIES})
	target_include_directories(${PROJECT_NAME} PRIVATE ${MOSQUITTO_INCLUDE_DIRS})

	gtest_suite_begin(tests-${PROJECT_NAME} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
	gtest_suite_sources(tests-${PROJECT_NAME}
		tests/TrazeProtocolTest.cpp
		TrazeProtocol.cpp
	)
	gtest_suite_deps(tests-${PROJECT_NAME} test-app uuidutil voxel ${MOSQUITTO_LIBRARIES})
	if (UNITTESTS)
		target_include_directories(tests-${PROJECT_NAME} PRIVATE ${MOSQUITTO_INCLUDE_DIRS})
	endif()
	gtest_suite_end(tests-${PROJECT_NAME})
else()
	message(WARNING "${PROJECT_NAME} is disabled - libmosquitto wasn't found")
endif()
