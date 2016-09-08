set(SRCS
    AABB.h
	App.cpp App.h
	ByteStream.cpp ByteStream.h
	Color.cpp Color.h
	Command.h Command.cpp
	Common.h
	Concurrency.h
	EnumHash.h
	EventBus.h
	GameConfig.h
	GLM.cpp GLM.h
	IFactoryRegistry.h
	Input.h
	Log.cpp Log.h
	MemoryAllocator.h
	MemGuard.cpp MemGuard.h
	NonCopyable.h
	Process.cpp Process.h
	QuadTree.h
	Random.cpp Random.h
	ReadWriteLock.h
	Rect.h
	Remotery.c Remotery.h
	Set.h
	Singleton.h
	String.cpp String.h
	ThreadPool.h
	TimeProvider.h TimeProvider.cpp
	Tokenizer.h Tokenizer.cpp
	Trace.cpp Trace.h
	Var.cpp Var.h
)
set(LIB core)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} io glm ${CMAKE_DL_LIBS})
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/AbstractTest.cpp
	tests/CoreTest.cpp
	tests/RectTest.cpp
	tests/ByteStreamTest.cpp
	tests/ThreadPoolTest.cpp
	tests/EventBusTest.cpp
	tests/QuadTreeTest.cpp
	tests/VarTest.cpp
	tests/CommandTest.cpp
	tests/SetTest.cpp
	tests/TokenizerTest.cpp
)
gtest_suite_deps(tests ${LIB})
