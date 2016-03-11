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
		String.h
		Input.h
		QuadTree.h
		Singleton.h
		Set.h
		Log.cpp Log.h
		Var.cpp Var.h
		Trace.h
		ThreadPool.h ThreadPool.cpp
		Tokenizer.h
		TimeProvider.h TimeProvider.cpp
	)
	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIRS})
	fips_deps(io ${SDL2_LIBRARIES})
fips_end_module()

gtest_begin(core)
	fips_dir(tests)
	fips_files(
		RectTest.cpp
		ByteStreamTest.cpp
		ThreadPoolTest.cpp
		EventBusTest.cpp
		QuadTreeTest.cpp
		VarTest.cpp
		CommandTest.cpp
		SetTest.cpp
		TokenizerTest.cpp
	)
	fips_deps(core)
gtest_end()
