set(SRCS
	command/ActionButton.h command/ActionButton.cpp
	command/CommandHandler.h command/CommandHandler.cpp
	command/CommandCompleter.h command/CommandCompleter.cpp
	command/Command.h command/Command.cpp

	metric/Metric.h metric/Metric.cpp
	metric/UDPMetricSender.h metric/UDPMetricSender.cpp
	metric/IMetricSender.h
	metric/MetricEvent.h

	io/EventHandler.cpp io/EventHandler.h
	io/File.cpp io/File.h
	io/FileStream.cpp io/FileStream.h
	io/Filesystem.cpp io/Filesystem.h
	io/IEventObserver.h
	io/IOResource.h

	collection/Array.h
	collection/ConcurrentQueue.h
	collection/ConcurrentSet.h
	collection/List.h
	collection/Map.h
	collection/Set.h

	ArrayLength.h
	Assert.cpp Assert.h
	App.cpp App.h
	AppCommand.cpp AppCommand.h
	BindingContext.cpp BindingContext.h
	ByteStream.cpp ByteStream.h
	Color.cpp Color.h
	Common.h Common.cpp
	Concurrency.h Concurrency.cpp
	CommandlineApp.h CommandlineApp.cpp
	Enum.h
	EventBus.cpp EventBus.h
	GameConfig.h
	GLM.cpp GLM.h
	Hash.h
	IComponent.h
	Log.cpp Log.h
	MD5.cpp MD5.h
	PoolAllocator.h
	MemGuard.cpp MemGuard.h
	NonCopyable.h
	Password.h
	Process.cpp Process.h
	ReadWriteLock.h
	RecursiveReadWriteLock.h
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
	Zip.cpp Zip.h
)
set(LIB core)

if (WINDOWS)
	set(UUID_LIBRARIES rpcrt4)
	# All this windows.h insanities ... giving up on this module
	set_property(GLOBAL PROPERTY ${LIB}_NOUNITY TRUE)
elseif (APPLE)
	set(UUID_LIBRARIES "-framework CoreFoundation")
else()
	find_package(UUID)
endif()

set(LIBS zlib sdl2 glm libuv ${CMAKE_DL_LIBS} ${UUID_LIBRARIES})
if (NOT MSVC)
list(APPEND LIBS backward)
endif()
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${LIBS})

set(TEST_SRCS
	tests/AbstractTest.cpp
	tests/ByteStreamTest.cpp
	tests/ColorTest.cpp
	tests/CommandTest.cpp
	tests/CommandCompleterTest.cpp
	tests/CommandHandlerTest.cpp
	tests/ConcurrentQueueTest.cpp
	tests/CoreTest.cpp
	tests/EventBusTest.cpp
	tests/FilesystemTest.cpp
	tests/FileStreamTest.cpp
	tests/FileTest.cpp
	tests/ListTest.cpp
	tests/LogTest.cpp
	tests/MapTest.cpp
	tests/MD5Test.cpp
	tests/MetricTest.cpp
	tests/PoolAllocatorTest.cpp
	tests/ReadWriteLockTest.cpp
	tests/SetTest.cpp
	tests/StringTest.cpp
	tests/ThreadPoolTest.cpp
	tests/TokenizerTest.cpp
	tests/UUIDTest.cpp
	tests/VarTest.cpp
	tests/ZipTest.cpp
)
set(TEST_FILES
	tests/iotest.txt
	tests/tokenizer.string
)

gtest_suite_files(tests ${TEST_FILES})
gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})

set(BENCHMARK_SRCS
	benchmark/AbstractBenchmark.cpp
	benchmarks/CollectionBenchmark.cpp
)
engine_add_executable(TARGET benchmarks-${LIB} SRCS ${BENCHMARK_SRCS} NOINSTALL)
engine_target_link_libraries(TARGET benchmarks-${LIB} DEPENDENCIES benchmark ${LIB})
