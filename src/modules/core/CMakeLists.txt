fips_begin_module(core)
	fips_files(
		Rect.h
		App.cpp App.h
		ByteStream.cpp ByteStream.h
		Common.h
		Command.h Command.cpp
		EventBus.h
		IFactoryRegistry.h
		NonCopyable.h
		String.cpp String.h
		Input.h
		QuadTree.h
		Singleton.h
		Set.h
		Random.cpp Random.h
		Remotery.c Remotery.h
		Log.cpp Log.h
		Var.cpp Var.h
		Trace.h
		Process.cpp Process.h
		ThreadPool.h
		Tokenizer.h
		TimeProvider.h TimeProvider.cpp
	)
	fips_deps(io glm ${CMAKE_DL_LIBS})
fips_end_module()
set_target_properties(core PROPERTIES COMPILE_FLAGS "-DUSE_REMOTERY=1 -DRMT_USE_OPENGL")

gtest_suite_files(tests tests/AbstractTest.cpp tests/RectTest.cpp tests/ByteStreamTest.cpp tests/ThreadPoolTest.cpp tests/EventBusTest.cpp tests/QuadTreeTest.cpp tests/VarTest.cpp tests/CommandTest.cpp tests/SetTest.cpp tests/TokenizerTest.cpp)
gtest_suite_deps(tests core)
