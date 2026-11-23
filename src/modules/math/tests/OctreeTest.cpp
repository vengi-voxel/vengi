/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "math/Octree.h"
#include "math/AABB.h"
#include "math/Frustum.h"
#include "core/Log.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Print.h"
#include "math/Scalar.h"

namespace math {

namespace oc {
class Item {
private:
	AABB<int> _bounds;
	int _id;
public:
	Item(const AABB<int>& bounds, int id) :
			_bounds(bounds), _id(id) {
	}

	const AABB<int>& aabb() const {
		return _bounds;
	}

	bool operator==(const Item& rhs) const {
		return rhs._id == _id;
	}
};
}

class OctreeTest : public app::AbstractTest {
public:
	void test(const math::Vec3f& mins, const math::Vec3f& maxs, int& n, const math::Vec3i& expectedMins, const math::Vec3i& expectedMaxs, int size) {
		EXPECT_TRUE(math::isPowerOfTwo(size));
		n = 0;
		const math::AABB<int> aabb(mins, maxs);
		Octree<oc::Item> octree(aabb);
		math::Frustum frustum(mins, maxs);
		const math::AABB<float>& frustumAABB = frustum.aabb();
		EXPECT_TRUE(math::all(math::epsilonEqual(mins, frustumAABB.mins(), 0.1f)));
		EXPECT_TRUE(math::all(math::epsilonEqual(maxs, frustumAABB.maxs(), 0.1f)));
		bool ignore = false;
		const math::AABB<int>& visitAABB = computeAABB(frustum, math::Vec3f(size));
		EXPECT_EQ(expectedMaxs, visitAABB.maxs());
		EXPECT_EQ(expectedMins, visitAABB.mins());
		octree.visit(frustum, [&] (const math::Vec3i& currentMins, const math::Vec3i& currentMaxs) {
			if (!ignore) {
				const math::Vec3i center = (currentMins + currentMaxs) / 2;
				for (int i = 0; i < center.length(); ++i) {
					const int mod = math::abs(center[i]) % size;
					const int expected = size / 2;
					EXPECT_EQ(mod, expected) << center[i] << " => " << center.x << ":" << center.y << ":" << center.z;
					ignore = mod != expected;
					if (ignore) {
						break;
					}
				}
			}
			++n;
			return true;
		}, math::Vec3i(size));
	}

	void testAABB(const math::Vec3f& mins, const math::Vec3f& maxs, const math::Vec3i& expectedMins, const math::Vec3i& expectedMaxs, int size) {
		EXPECT_TRUE(math::isPowerOfTwo(size));
		const math::AABB<int> aabb(mins, maxs);
		math::Frustum frustum(mins, maxs);
		const math::AABB<float>& frustumAABB = frustum.aabb();
		EXPECT_TRUE(math::all(math::epsilonEqual(mins, frustumAABB.mins(), 0.1f)));
		EXPECT_TRUE(math::all(math::epsilonEqual(maxs, frustumAABB.maxs(), 0.1f)));
		const math::AABB<int>& visitAABB = computeAABB(frustum, math::Vec3f(size));
		EXPECT_EQ(expectedMaxs, visitAABB.maxs());
		EXPECT_EQ(expectedMins, visitAABB.mins());
	}
};

TEST_F(OctreeTest, testAdd) {
	Octree<oc::Item, int> octree({0, 0, 0, 100, 100, 100});
	EXPECT_EQ(0, octree.count())<< "Expected to have no entries in the octree";
	EXPECT_TRUE(octree.insert({{51, 51, 51, 53, 53, 53}, 1}));
	EXPECT_EQ(1, octree.count())<< "Expected to have 1 entry in the octree";
	EXPECT_TRUE(octree.insert({{15, 15, 15, 18, 18, 18}, 2}));
	EXPECT_EQ(2, octree.count())<< "Expected to have 2 entries in the octree";
}

TEST_F(OctreeTest, testAddAABBTooBig) {
	Octree<oc::Item, int> octree({0, 0, 0, 100, 100, 100});
	EXPECT_EQ(0, octree.count())<< "Expected to have no entries in the octree";
	EXPECT_FALSE(octree.insert({{-100, -100, -100, 200, 200, 200}, 1}));
}

TEST_F(OctreeTest, testRemove) {
	Octree<oc::Item, int> octree({0, 0, 0, 100, 100, 100});
	EXPECT_EQ(0, octree.count())<< "Expected to have no entries in the octree";
	const oc::Item item({51, 51, 51, 53, 53, 53}, 1);
	EXPECT_TRUE(octree.insert(item));
	const oc::Item item2({52, 52, 52, 54, 55, 55}, 2);
	EXPECT_TRUE(octree.insert(item2));
	EXPECT_EQ(2, octree.count())<< "Expected to have 2 entries in the octree";
	EXPECT_TRUE(octree.remove(item));
	EXPECT_EQ(1, octree.count())<<"Expected to have 0 entries in the octree";
}

TEST_F(OctreeTest, testQuery) {
	Octree<oc::Item, int> octree({0, 0, 0, 100, 100, 100}, 3);
	{
		Octree<oc::Item, int>::Contents contents;
		octree.query({50, 50, 50, 60, 60, 60}, contents);
		EXPECT_EQ(0u, contents.size())<<"Expected to find nothing in an empty tree";
	}
	{
		Octree<oc::Item, int>::Contents contents;
		octree.query({52, 52, 52, 54, 54, 54}, contents);
		EXPECT_EQ(0u, contents.size())<<"Expected to find nothing in an empty tree";
	}
	oc::Item item1({51, 51, 51, 53, 53, 53}, 1);
	EXPECT_TRUE(octree.insert(item1));
	{
		Octree<oc::Item, int>::Contents contents;
		octree.query(item1.aabb(), contents);
		EXPECT_EQ(1u, contents.size())<<"Expected to find one entry for the item aabb";
	}
	{
		Octree<oc::Item, int>::Contents contents;
		octree.query({52, 52, 52, 54, 54, 54}, contents);
		EXPECT_EQ(1u, contents.size())<<"Expected to find one entry for the overlapping aabb";
	}
	{
		Octree<oc::Item, int>::Contents contents;
		octree.query({50, 50, 50, 52, 52, 52}, contents);
		EXPECT_TRUE(intersects(item1.aabb(), {50, 50, 50, 52, 52, 52}));
		EXPECT_EQ(1u, contents.size())<<"Expected to find one entry for the overlapping aabb";
	}
}

TEST_F(OctreeTest, testOctreeVisitOrthoFrustum) {
	const math::Vec3f mins(0.0f);
	const math::Vec3f maxs(128.0f);
	const int slices = 8;
	const math::AABB<int> aabb(mins, maxs);
	Octree<oc::Item> octree(aabb);
	math::Frustum frustum(mins, maxs);
	const math::AABB<float>& frustumAABB = frustum.aabb();
	ASSERT_EQ(mins, frustumAABB.mins());
	ASSERT_EQ(maxs, frustumAABB.maxs());
	int n = 0;
	const int blockSize = math::ceil(aabb.getWidthX() / slices);
	octree.visit(frustum, [&] (const math::Vec3i& currentMins, const math::Vec3i& currentMaxs) {
		const math::Vec3i center = (currentMins + currentMaxs) / 2;
		for (int i = 0; i < center.length(); ++i) {
			EXPECT_EQ(center[i] % blockSize, slices) << center.x << ":" << center.y << ":" << center.z;
		}
		++n;
		return true;
	}, math::Vec3i(blockSize));
	EXPECT_EQ(math::pow(slices, math::Vec3i::length()), n);
}

TEST_F(OctreeTest, testOctreeComputeAABB1) {
	testAABB(math::Vec3f(-4.0f), math::Vec3f(6.0f), math::Vec3i(-32), math::Vec3i(32), 32);
}

TEST_F(OctreeTest, testOctreeComputeAABB2) {
	testAABB(math::Vec3f(1.0f), math::Vec3f(6.0f), math::Vec3i(0), math::Vec3i(32), 32);
}

TEST_F(OctreeTest, testOctreeComputeAABB3) {
	testAABB(
		math::Vec3f(-34.0f, -12.0f, -1.0f),
		math::Vec3f(19.0f, 17.0f, 33.0f),

		math::Vec3i(-64, -32, -32),
		math::Vec3i(64, 32, 96),

		32);
}

TEST_F(OctreeTest, testOctreeVisitOrthoFrustumNoPerfectMatch) {
	const math::Vec3f mins(-4.0f);
	const math::Vec3f maxs(6.0f);
	int n = 0;
	test(mins, maxs, n, math::Vec3i(-32), math::Vec3i(32), 32);
	EXPECT_EQ(n, 8);
}

TEST_F(OctreeTest, testOctreeVisitOrthoFrustumNoPerfectMatchJustOneField) {
	const math::Vec3f mins(1.0f);
	const math::Vec3f maxs(6.0f);
	int n = 0;
	test(mins, maxs, n, math::Vec3i(0), math::Vec3i(32), 32);
	EXPECT_EQ(n, 1);
}

TEST_F(OctreeTest, testOctreeVisitOrthoFrustumNoPerfectMatchBigAndUneven) {
	const math::Vec3f mins(-34.0f, -12.0f, -1.0f);
	const math::Vec3f maxs(19.0f, 17.0f, 33.0f);
	int n = 0;
	test(mins, maxs, n, math::Vec3i(-64, -32, -32), math::Vec3i(64, 32, 96), 32);
	EXPECT_LE(n, 4 * 2 * 4) << "More than the absolute maxs of the calculated max AABB";
	EXPECT_EQ(n, 18) << "Expected to get some bboxes not visited, because they are outside of the original frustum";
}

}
