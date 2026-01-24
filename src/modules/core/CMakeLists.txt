set(SRCS
	collection/Array.h
	collection/Array2DView.h
	collection/Array3DView.h
	collection/BitSet.h
	collection/Buffer.h
	collection/BufferView.h
	collection/ConcurrentDynamicArray.h
	collection/ConcurrentQueue.h
	collection/ConcurrentPriorityQueue.h
	collection/ConcurrentSet.h
	collection/DynamicArray.h
	collection/DynamicParallelMap.h
	collection/DynamicList.h
	collection/DynamicMap.h
	collection/DynamicStack.h
	collection/DynamicStringMap.h
	collection/Functions.h
	collection/List.h
	collection/Map.h collection/Map.cpp
	collection/ParallelMap.h
	collection/Set.h
	collection/Stack.h
	collection/StringMap.h
	collection/StringSet.h
	collection/Vector.h

	concurrent/Atomic.cpp concurrent/Atomic.h
	concurrent/Concurrency.h concurrent/Concurrency.cpp
	concurrent/ConditionVariable.h concurrent/ConditionVariable.cpp
	concurrent/Lock.cpp concurrent/Lock.h
	concurrent/ThreadPool.cpp concurrent/ThreadPool.h
	concurrent/Thread.cpp concurrent/Thread.h
	concurrent/Future.h

	external/strnatcmp.c external/strnatcmp.h

	sdl/SDLSystem.h

	Algorithm.cpp Algorithm.h
	Alphanumeric.cpp Alphanumeric.h
	ArrayLength.h
	Assert.cpp Assert.h
	BindingContext.cpp BindingContext.h
	Bits.h
	Common.cpp Common.h
	DirtyState.h
	Endian.h
	Enum.h
	FixedString.h
	FourCC.h
	ConfigVar.h
	GLM.cpp GLM.h
	GLMConst.h
	Hash.cpp Hash.h
	IComponent.h
	Log.cpp Log.h
	MD5.cpp MD5.h
	NonCopyable.h
	Optional.h
	Pair.h
	Path.cpp Path.h
	PoolAllocator.h
	Process.cpp Process.h
	ResultType.h
	SharedPtr.h
	Singleton.h
	StandardLib.h
	String.cpp String.h
	StringUtil.cpp StringUtil.h
	TimeProvider.h TimeProvider.cpp
	TimedValue.h
	Tokenizer.h Tokenizer.cpp
	Trace.cpp Trace.h
	Tuple.h
	Unicode.cpp Unicode.h
	UUID.cpp UUID.h
	Var.cpp Var.h
)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tracy/public/TracyClient.cpp)
	list(APPEND SRCS tracy/public/TracyClient.cpp)
	message(STATUS "Building with tracy enabled")
	set_source_files_properties(tracy/public/TracyClient.cpp PROPERTIES
		COMPILE_FLAGS "$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wno-empty-body -Wno-missing-field-initializers -Wno-shadow>"
	)
else()
	message(STATUS "Building with tracy disabled")
endif()

engine_source_optimize(Algorithm.cpp)

set(LIB core)
set(SDLLIB)
if (USE_SDL_VERSION EQUAL 2)
	set(SDLLIB sdl2)
elseif (USE_SDL_VERSION EQUAL 3)
	set(SDLLIB sdl3)
else()
	message(FATAL_ERROR "Invalid SDL version")
endif()

set(LIBS ${SDLLIB} glm ${CMAKE_DL_LIBS})

if (USE_STACKTRACES)
	list(APPEND LIBS backward)
endif()

engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${LIBS})
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tracy/public/TracyClient.cpp)
	target_compile_definitions(${LIB} PUBLIC TRACY_ENABLE TRACY_ON_DEMAND)
endif()

if (WINDOWS)
	# All this windows.h insanities ... giving up on this module
	set_target_properties(${LIB} PROPERTIES UNITY_BUILD FALSE)
endif()

if (USE_STACKTRACES)
	target_compile_definitions(${LIB} PRIVATE HAVE_BACKWARD)
endif()

set(TEST_SRCS
	tests/TestHelper.h
	tests/AlgorithmTest.cpp
	tests/AlphanumericTest.cpp
	tests/ArrayTest.cpp
	tests/BitsTest.cpp
	tests/BitSetTest.cpp
	tests/BufferTest.cpp
	tests/ConcurrentDynamicArrayTest.cpp
	tests/ConcurrentPriorityQueueTest.cpp
	tests/ConcurrentQueueTest.cpp
	tests/CoreTest.cpp
	tests/DynamicArrayTest.cpp
	tests/DynamicListTest.cpp
	tests/DynamicStackTest.cpp
	tests/HashTest.cpp
	tests/ListTest.cpp
	tests/MapTest.cpp
	tests/DynamicMapTest.cpp
	tests/MD5Test.cpp
	tests/OptionalTest.cpp
	tests/PathTest.cpp
	tests/PoolAllocatorTest.cpp
	tests/ProcessTest.cpp
	tests/QueueTest.cpp
	tests/RingBufferTest.cpp
	tests/SharedPtrTest.cpp
	tests/StackTest.cpp
	tests/StringTest.cpp
	tests/StringUtilTest.cpp
	tests/ThreadPoolTest.cpp
	tests/TokenizerTest.cpp
	tests/TupleTest.cpp
	tests/UnicodeTest.cpp
	tests/UUIDTest.cpp
	tests/VarTest.cpp
	tests/VectorTest.cpp
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} io)
gtest_suite_end(tests-${LIB})

if (USE_BENCHMARKS)
	set(BENCHMARK_SRCS
		benchmarks/CollectionBenchmark.cpp
		benchmarks/StringBenchmark.cpp
	)
	engine_add_executable(TARGET benchmarks-${LIB} SRCS ${BENCHMARK_SRCS} NOINSTALL)
	engine_target_link_libraries(TARGET benchmarks-${LIB} DEPENDENCIES benchmark-app)
endif()
