set(SRCS
	collection/Array.h
	collection/Buffer.h
	collection/ConcurrentQueue.h
	collection/ConcurrentSet.h
	collection/DynamicArray.h
	collection/Functions.h
	collection/List.h
	collection/Map.h
	collection/Set.h
	collection/SetUtil.h
	collection/Stack.h
	collection/StringMap.h
	collection/StringSet.h
	collection/Vector.h

	concurrent/Atomic.cpp concurrent/Atomic.h
	concurrent/Concurrency.h concurrent/Concurrency.cpp
	concurrent/ConditionVariable.h concurrent/ConditionVariable.cpp
	concurrent/Lock.cpp concurrent/Lock.h
	concurrent/ReadWriteLock.cpp concurrent/ReadWriteLock.h
	concurrent/ThreadPool.cpp concurrent/ThreadPool.h

	Algorithm.h
	ArrayLength.h
	Assert.cpp Assert.h
	BindingContext.cpp BindingContext.h
	ByteStream.cpp ByteStream.h
	Color.cpp Color.h
	Common.cpp Common.h
	Enum.h
	EventBus.cpp EventBus.h
	FourCC.h
	GameConfig.h
	GLM.cpp GLM.h
	GLMConst.h
	Hash.h
	IComponent.h
	Log.cpp Log.h
	MD5.cpp MD5.h
	PoolAllocator.h
	MemGuard.cpp MemGuard.h
	NonCopyable.h
	Password.h
	Process.cpp Process.h
	SharedPtr.h
	Singleton.h
	StandardLib.h
	String.cpp String.h
	StringUtil.cpp StringUtil.h
	TimeProvider.h TimeProvider.cpp
	Tokenizer.h Tokenizer.cpp
	Trace.cpp Trace.h
	UTF8.cpp UTF8.h
	Var.cpp Var.h
	Vector.h
	Zip.cpp Zip.h miniz.h miniz.c
)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tracy/TracyClient.cpp)
	list(APPEND SRCS tracy/TracyClient.cpp)
	message(STATUS "Building with tracy enabled")
else()
	message(STATUS "Building with tracy disabled")
endif()

set(LIB core)

if (WINDOWS)
	# All this windows.h insanities ... giving up on this module
	set_property(GLOBAL PROPERTY ${LIB}_NOUNITY TRUE)
endif()

set(LIBS sdl2 glm backward ${CMAKE_DL_LIBS})
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${LIBS})
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tracy/TracyClient.cpp)
	target_compile_definitions(${LIB} PUBLIC TRACY_ENABLE TRACY_ON_DEMAND)
endif()

set(TEST_SRCS
	tests/TestHelper.h
	tests/AlgorithmTest.cpp
	tests/ArrayTest.cpp
	tests/BufferTest.cpp
	tests/ByteStreamTest.cpp
	tests/ColorTest.cpp
	tests/ConcurrentQueueTest.cpp
	tests/CoreTest.cpp
	tests/DynamicArrayTest.cpp
	tests/EventBusTest.cpp
	tests/ListTest.cpp
	tests/LogTest.cpp
	tests/MapTest.cpp
	tests/MD5Test.cpp
	tests/PoolAllocatorTest.cpp
	tests/ReadWriteLockTest.cpp
	tests/SetUtilTest.cpp
	tests/SharedPtrTest.cpp
	tests/StackTest.cpp
	tests/StringTest.cpp
	tests/StringUtilTest.cpp
	tests/ThreadPoolTest.cpp
	tests/TokenizerTest.cpp
	tests/VarTest.cpp
	tests/VectorTest.cpp
	tests/ZipTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})

set(BENCHMARK_SRCS
	benchmarks/CollectionBenchmark.cpp
)
engine_add_executable(TARGET benchmarks-${LIB} SRCS ${BENCHMARK_SRCS} NOINSTALL)
engine_target_link_libraries(TARGET benchmarks-${LIB} DEPENDENCIES benchmark-app)
