set(SRCS
	AABB.h
	Axis.h
	Bezier.h
	Frustum.cpp Frustum.h
	Functions.cpp Functions.h
	Octree.h Octree.cpp
	OctreeCache.h
	Plane.h Plane.cpp
	QuadTree.h
	QuadTreeCache.h
	Random.cpp Random.h
	Ray.h
	Rect.h
)
set(LIB math)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core)

set(TEST_SRCS
	tests/AABBTest.cpp
	tests/FrustumTest.cpp
	tests/OctreeTest.cpp
	tests/PlaneTest.cpp
	tests/QuadTreeTest.cpp
	tests/RectTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB} test-app)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})
