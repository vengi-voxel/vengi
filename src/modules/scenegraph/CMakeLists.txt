set(LIB scenegraph)
set(SRCS
	Clipper.h Clipper.cpp
	CoordinateSystem.h
	CoordinateSystemUtil.h CoordinateSystemUtil.cpp
	FrameTransform.h FrameTransform.cpp
	JsonExporter.h JsonExporter.cpp
	SceneGraph.h SceneGraph.cpp
	SceneGraphAnimation.h
	SceneGraphKeyFrame.h
	SceneGraphNode.h SceneGraphNode.cpp
	SceneGraphNodeCamera.h SceneGraphNodeCamera.cpp
	SceneGraphNodeProperties.h
	SceneGraphTransform.h SceneGraphTransform.cpp
	SceneGraphUtil.h SceneGraphUtil.cpp
	SceneUtil.h SceneUtil.cpp
	SceneGraphListener.h
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxelutil)

set(TEST_SRCS
	tests/CoordinateSystemTest.cpp
	tests/SceneGraphTest.cpp
	tests/ClipperTest.cpp
	tests/SceneGraphUtilTest.cpp
	tests/TestHelper.h
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB} test-app video)
gtest_suite_end(tests-${LIB})

set(BENCHMARK_SRCS
	benchmarks/ClipperBenchmark.cpp
	benchmarks/FrameTransformBenchmark.cpp
	benchmarks/SceneGraphBenchmark.cpp
)
engine_add_executable(TARGET benchmarks-${LIB} SRCS ${BENCHMARK_SRCS} NOINSTALL)
engine_target_link_libraries(TARGET benchmarks-${LIB} DEPENDENCIES benchmark-app ${LIB})
