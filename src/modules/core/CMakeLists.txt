set(SRCS
	Rect.h
	App.cpp App.h
	ByteStream.cpp ByteStream.h
	Common.h
	Command.h Command.cpp
	Color.cpp Color.h
	Concurrency.h
	EventBus.h
	IFactoryRegistry.h
	NonCopyable.h
	String.cpp String.h
	Input.h
	QuadTree.h
	Singleton.h
	Set.h
	Random.cpp Random.h
	Log.cpp Log.h
	Var.cpp Var.h
	Remotery.c Remotery.h
	Trace.cpp Trace.h
	Process.cpp Process.h
	ThreadPool.h
	Tokenizer.h
	TimeProvider.h TimeProvider.cpp
)
set(LIB core)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} io glm ${CMAKE_DL_LIBS})
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/AbstractTest.cpp
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
