set(SRCS
	AABB.h
	Axis.h
	Bezier.h
	Frustum.cpp Frustum.h
	Octree.h Octree.cpp
	Plane.h Plane.cpp
	QuadTree.h
	Random.cpp Random.h
	Rect.h
)
set(LIB math)
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES core)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

set(TEST_SRCS
	tests/AABBTest.cpp
	tests/FrustumTest.cpp
	tests/OctreeTest.cpp
	tests/PlaneTest.cpp
	tests/QuadTreeTest.cpp
	tests/RectTest.cpp
)

gtest_suite_files(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-math TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_files(tests-math ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_deps(tests-math ${LIB})
gtest_suite_end(tests-math)
