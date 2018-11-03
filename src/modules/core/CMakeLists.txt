set(SRCS
	command/ActionButton.h command/ActionButton.cpp
	command/CommandHandler.h command/CommandHandler.cpp
	command/Command.h command/Command.cpp
	Assert.h
	App.cpp App.h
	AppCommand.cpp AppCommand.h
	ByteStream.cpp ByteStream.h
	Color.cpp Color.h
	Common.h
	Concurrency.h Concurrency.cpp
	ConsoleApp.h ConsoleApp.cpp
	EventBus.cpp EventBus.h
	GameConfig.h
	GLM.cpp GLM.h
	Hash.h
	IComponent.h
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
	UUID.cpp UUID.h
	Var.cpp Var.h
	Vector.h
	Vertex.h
	Zip.cpp Zip.h
)
set(LIB core)
set(FILES
	shared/autoexec.cfg
)

find_package(UUID)

engine_add_module(TARGET ${LIB} SRCS ${SRCS} FILES ${FILES} DEPENDENCIES collection io metric restclient-cpp zlib ${CMAKE_DL_LIBS} ${UUID_LIBRARIES})

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
	tests/UUIDTest.cpp
	tests/ZipTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})
