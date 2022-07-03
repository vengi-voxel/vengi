project(thumbnailer)
set(SRCS
	Thumbnailer.h Thumbnailer.cpp
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS})
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES app voxelrender)

if (UNIX)
	enable_testing()
	configure_file(tests/tests.sh.in ${PROJECT_NAME}-tests.sh @ONLY)
	add_test(NAME shelltests-${PROJECT_NAME} COMMAND ${PROJECT_NAME}-tests.sh $<TARGET_FILE:${PROJECT_NAME}>)
	set_tests_properties(shelltests-${PROJECT_NAME} PROPERTIES DEPENDS ${PROJECT_NAME})
endif()
