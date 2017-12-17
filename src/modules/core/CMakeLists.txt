set(SRCS
	command/CommandHandler.h command/CommandHandler.cpp
	command/Command.h command/Command.cpp
	Assert.h
	App.cpp App.h
	AppCommand.cpp AppCommand.h
	ByteStream.cpp ByteStream.h
	Color.cpp Color.h
	Common.h
	Concurrency.h
	ConsoleApp.h ConsoleApp.cpp
	EventBus.cpp EventBus.h
	GameConfig.h
	GLM.cpp GLM.h
	Hash.h
	IFactoryRegistry.h
	Input.cpp Input.h
	JSON.h json.hpp
	Log.cpp Log.h
	MD5.cpp MD5.h
	PoolAllocator.h
	MemGuard.cpp MemGuard.h
	NonCopyable.h
	Password.h
	Process.cpp Process.h
	ReadWriteLock.h
	RecursiveReadWriteLock.h
	Rest.h Rest.cpp
	Singleton.h
	String.cpp String.h
	ThreadPool.cpp ThreadPool.h
	TimeProvider.h TimeProvider.cpp
	Tokenizer.h Tokenizer.cpp
	Trace.cpp Trace.h
	UTF8.cpp UTF8.h
	Var.cpp Var.h
	Vector.h
	Vertex.h
	Zip.h
)
set(LIB core)
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES collection io restclient-cpp zlib ${CMAKE_DL_LIBS})
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

set(TEST_SRCS
	tests/AbstractTest.cpp
	tests/CoreTest.cpp
	tests/ByteStreamTest.cpp
	tests/ThreadPoolTest.cpp
	tests/EventBusTest.cpp
	tests/VarTest.cpp
	tests/CommandTest.cpp
	tests/CommandHandlerTest.cpp
	tests/MD5Test.cpp
	tests/LogTest.cpp
	tests/TokenizerTest.cpp
	tests/PoolAllocatorTest.cpp
	tests/StringTest.cpp
	tests/ReadWriteLockTest.cpp
)

gtest_suite_files(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_files(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})
