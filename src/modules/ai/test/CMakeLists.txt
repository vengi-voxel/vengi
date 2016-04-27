enable_testing()

include_directories(
	${SIMPLEAI_SOURCE_DIR}/src/ai
	${SIMPLEAI_SOURCE_DIR}/src/test/gtest ${SIMPLEAI_SOURCE_DIR}/src/test/gtest/include
)
set(SRC
	AggroTest.cpp AggroTest.h
	GeneralTest.cpp GeneralTest.h
	GroupTest.cpp GroupTest.h
	MessageTest.cpp MessageTest.h
	MovementTest.cpp MovementTest.h
	NodeTest.cpp NodeTest.h
	ParserTest.cpp ParserTest.h
	TestAll.cpp
	TestEntity.h
	TestShared.cpp TestShared.h
	ThreadTest.cpp ThreadTest.h
	ZoneTest.cpp ZoneTest.h

	gtest/src/gtest.cc
	gtest/src/gtest-death-test.cc
	gtest/src/gtest-filepath.cc
	gtest/src/gtest-port.cc
	gtest/src/gtest-printers.cc
	gtest/src/gtest-test-part.cc
	gtest/src/gtest-typed-test.cc
)

if (SIMPLEAI_LUA)
	list(APPEND SRC LUATreeLoaderTest.cpp LUATreeLoaderTest.h)
endif()

if (SIMPLEAI_XML)
	list(APPEND SRC XMLTreeLoaderTest.cpp XMLTreeLoaderTest.h)
endif()

add_executable(simpleai-tests ${SRC})
target_link_libraries(simpleai-tests simpleai)
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
