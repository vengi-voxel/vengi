set(SRCS
	AABB.h
	Axis.cpp Axis.h
	Bezier.h
	CoordinateSystem.h
	CoordinateSystemUtil.cpp CoordinateSystemUtil.h
	Easing.h
	Frustum.cpp Frustum.h
	Functions.cpp Functions.h
	OBB.h
	Octree.h Octree.cpp
	Plane.h Plane.cpp
	QuadTree.h
	Random.cpp Random.h
	Ray.h
	Rect.h
	Tri.h Tri.cpp
	Math.h Math.cpp
)
set(LIB math)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core color)

set(TEST_SRCS
	tests/AABBTest.cpp
	tests/CoordinateSystemTest.cpp
	tests/FrustumTest.cpp
	tests/MathTest.cpp
	tests/OBBTest.cpp
	tests/OctreeTest.cpp
	tests/PlaneTest.cpp
	tests/QuadTreeTest.cpp
	tests/RectTest.cpp
	tests/TriTest.cpp
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})

set(BENCHMARK_SRCS
	benchmarks/OBBBenchmark.cpp
	benchmarks/TriBenchmark.cpp
)
engine_add_executable(TARGET benchmarks-${LIB} SRCS ${BENCHMARK_SRCS} NOINSTALL)
engine_target_link_libraries(TARGET benchmarks-${LIB} DEPENDENCIES benchmark-app ${LIB})
