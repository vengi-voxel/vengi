project(palconvert)
set(SRCS
	PalConvert.h PalConvert.cpp
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} DESCRIPTION "Command line palette tool")
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES app palette)

enable_testing()
configure_file(tests/tests.sh.in ${PROJECT_NAME}-tests.sh @ONLY)
configure_file(tests/tests.bat.in ${PROJECT_NAME}-tests.bat @ONLY)
if (WIN32)
	add_test(NAME shelltests-${PROJECT_NAME} COMMAND ${PROJECT_NAME}-tests.bat $<TARGET_FILE:${PROJECT_NAME}>)
else()
	add_test(NAME shelltests-${PROJECT_NAME} COMMAND ${PROJECT_NAME}-tests.sh $<TARGET_FILE:${PROJECT_NAME}>)
endif()
set_tests_properties(shelltests-${PROJECT_NAME} PROPERTIES DEPENDS ${PROJECT_NAME})
