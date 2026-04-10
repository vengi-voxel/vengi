project(palconvert)
set(SRCS
	PalConvert.h PalConvert.cpp
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} DESCRIPTION "Command line palette tool")
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES app palette)

engine_generate_standalone_wix(
	TARGET ${PROJECT_NAME}
	TEMPLATE ${ROOT_DIR}/contrib/installer/windows/application.wxs.in
	OUTPUT ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.wxs
	PRODUCT_NAME "Vengi Palconvert"
	UPGRADE_GUID "A1B2C3D4-E5F6-4A7B-8C9D-0E1F2A3B4C5D"
)

enable_testing()
configure_file(tests/tests.sh.in ${PROJECT_NAME}-tests.sh @ONLY)
configure_file(tests/tests.bat.in ${PROJECT_NAME}-tests.bat @ONLY)
if (WIN32)
	add_test(NAME shelltests-${PROJECT_NAME} COMMAND ${PROJECT_NAME}-tests.bat $<TARGET_FILE:${PROJECT_NAME}>)
else()
	add_test(NAME shelltests-${PROJECT_NAME} COMMAND ${PROJECT_NAME}-tests.sh $<TARGET_FILE:${PROJECT_NAME}>)
endif()
set_tests_properties(shelltests-${PROJECT_NAME} PROPERTIES DEPENDS ${PROJECT_NAME})
