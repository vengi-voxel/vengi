set(SRCS
	AABB.h
	Axis.h
	command/CommandHandler.h command/CommandHandler.cpp
	command/Command.h command/Command.cpp
	Assert.h
	App.cpp App.h
	Bezier.h
	ByteStream.cpp ByteStream.h
	Color.cpp Color.h
	ConcurrentQueue.h
	ConcurrentSet.h
	ConcurrentVector.h
	Common.h
	Concurrency.h
	ConsoleApp.h ConsoleApp.cpp
	EventBus.cpp EventBus.h
	Frustum.cpp Frustum.h
	GameConfig.h
	GLM.cpp GLM.h
	Hash.h
	IFactoryRegistry.h
	Input.cpp Input.h
	JSON.h json.hpp
	Log.cpp Log.h
	MD5.cpp MD5.h
	MemoryAllocator.h
	MemGuard.cpp MemGuard.h
	MurmurHash3.cpp MurmurHash3.h
	NonCopyable.h
	Octree.h Octree.cpp
	Password.h
	Plane.h Plane.cpp
	Process.cpp Process.h
	QuadTree.h
	Random.cpp Random.h
	ReadWriteLock.h
	Rect.h
	RecursiveReadWriteLock.h
	Rest.h Rest.cpp
	Set.h
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
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES io restclient-cpp zlib ${CMAKE_DL_LIBS})
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

set(TEST_SRCS
	tests/AbstractTest.cpp
	tests/AABBTest.cpp
	tests/CoreTest.cpp
	tests/ConcurrentQueueTest.cpp
	tests/RectTest.cpp
	tests/ByteStreamTest.cpp
	tests/ThreadPoolTest.cpp
	tests/EventBusTest.cpp
	tests/QuadTreeTest.cpp
	tests/OctreeTest.cpp
	tests/VarTest.cpp
	tests/CommandTest.cpp
	tests/MD5Test.cpp
	tests/SetTest.cpp
	tests/TokenizerTest.cpp
	tests/FrustumTest.cpp
	tests/PlaneTest.cpp
	tests/StringTest.cpp
	tests/ReadWriteLockTest.cpp
)

gtest_suite_files(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-core TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_files(tests-core ${TEST_SRCS})
gtest_suite_deps(tests-core ${LIB})
gtest_suite_end(tests-core)
