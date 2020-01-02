/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "math/Octree.h"
#include "math/AABB.h"
#include "math/Frustum.h"
#include "core/Log.h"
#include "core/GLM.h"
#include "math/OctreeCache.h"

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

class OctreeTest : public core::AbstractTest {
public:
	void test(const glm::vec3& mins, const glm::vec3& maxs, int& n, const glm::ivec3& expectedMins, const glm::ivec3& expectedMaxs, int size) {
		EXPECT_TRUE(glm::isPowerOfTwo(size));
		n = 0;
		const math::AABB<int> aabb(mins, maxs);
		Octree<oc::Item> octree(aabb);
		math::Frustum frustum(aabb);
		const math::AABB<float>& frustumAABB = frustum.aabb();
		EXPECT_TRUE(glm::all(glm::epsilonEqual(mins, frustumAABB.mins(), 0.1f)))
			<< glm::to_string(mins) << ", "
			<< glm::to_string(frustumAABB.mins());
		EXPECT_TRUE(glm::all(glm::epsilonEqual(maxs, frustumAABB.maxs(), 0.1f)))
			<< glm::to_string(maxs) << ", "
			<< glm::to_string(frustumAABB.maxs());
		bool ignore = false;
		const math::AABB<int>& visitAABB = computeAABB(frustum, glm::vec3(size));
		EXPECT_EQ(expectedMaxs, visitAABB.maxs())
			<< "Expected to get " << glm::to_string(expectedMaxs)
			<< " but got " << glm::to_string(visitAABB.maxs());
		EXPECT_EQ(expectedMins, visitAABB.mins())
			<< "Expected to get " << glm::to_string(expectedMins)
			<< " but got " << glm::to_string(visitAABB.mins());
		octree.visit(frustum, [&] (const glm::ivec3& currentMins, const glm::ivec3& currentMaxs) {
			if (!ignore) {
				const glm::ivec3 center = (currentMins + currentMaxs) / 2;
				for (int i = 0; i < center.length(); ++i) {
					const int mod = glm::abs(center[i]) % size;
					const int expected = size / 2;
					EXPECT_EQ(mod, expected) << center[i] << " => " << glm::to_string(center);
					ignore = mod != expected;
					if (ignore) {
						break;
					}
				}
			}
			++n;
			return true;
		}, glm::ivec3(size));
	}

	void testAABB(const glm::vec3& mins, const glm::vec3& maxs, const glm::ivec3& expectedMins, const glm::ivec3& expectedMaxs, int size) {
		EXPECT_TRUE(glm::isPowerOfTwo(size));
		const math::AABB<int> aabb(mins, maxs);
		math::Frustum frustum(aabb);
		const math::AABB<float>& frustumAABB = frustum.aabb();
		EXPECT_TRUE(glm::all(glm::epsilonEqual(mins, frustumAABB.mins(), 0.1f)))
			<< glm::to_string(mins) << ", "
			<< glm::to_string(frustumAABB.mins());
		EXPECT_TRUE(glm::all(glm::epsilonEqual(maxs, frustumAABB.maxs(), 0.1f)))
			<< glm::to_string(maxs) << ", "
			<< glm::to_string(frustumAABB.maxs());
		const math::AABB<int>& visitAABB = computeAABB(frustum, glm::vec3(size));
		EXPECT_EQ(expectedMaxs, visitAABB.maxs())
			<< "Expected to get " << glm::to_string(expectedMaxs)
			<< " but got " << glm::to_string(visitAABB.maxs());
		EXPECT_EQ(expectedMins, visitAABB.mins())
			<< "Expected to get " << glm::to_string(expectedMins)
			<< " but got " << glm::to_string(visitAABB.mins());
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

TEST_F(OctreeTest, testOctreeCache) {
	Octree<oc::Item, int> octree({0, 0, 0, 100, 100, 100});
	OctreeCache<oc::Item, int> cache(octree);
	{
		Octree<oc::Item, int>::Contents contents;
		octree.query({50, 50, 50, 60, 60, 60}, contents);
		EXPECT_EQ(0u, contents.size())<<"Expected to find nothing in an empty tree";
		contents.clear();
		EXPECT_FALSE(cache.query({50, 50, 50, 60, 60, 60}, contents));
		contents.clear();
		EXPECT_TRUE(cache.query({50, 50, 50, 60, 60, 60}, contents));
	}
	oc::Item item({51, 51, 51, 53, 53, 53}, 1);
	EXPECT_TRUE(octree.insert(item));
	{
		Octree<oc::Item, int>::Contents contents;
		EXPECT_FALSE(cache.query({50, 50, 50, 60, 60, 60}, contents)) << "Expected to have the cache cleared, the octree was in a dirty state";
		EXPECT_EQ(1u, contents.size())<<"Expected to find one entry for the enclosing aabb";
	}
	{
		Octree<oc::Item, int>::Contents contents;
		octree.query({50, 50, 50, 52, 52, 52}, contents);
		EXPECT_EQ(1u, contents.size())<<"Expected to find one entry for the overlapping aabb";
		contents.clear();
		EXPECT_FALSE(cache.query({50, 50, 50, 52, 52, 52}, contents));
		contents.clear();
		EXPECT_TRUE(cache.query({50, 50, 50, 52, 52, 52}, contents));
	}
}

TEST_F(OctreeTest, testOctreeVisitOrthoFrustum) {
	const glm::vec3 mins(0.0f);
	const glm::vec3 maxs(128.0f);
	const int slices = 8;
	const math::AABB<int> aabb(mins, maxs);
	Octree<oc::Item> octree(aabb);
	math::Frustum frustum(aabb);
	const math::AABB<float>& frustumAABB = frustum.aabb();
	ASSERT_EQ(mins, frustumAABB.mins())
		<< glm::to_string(mins) << ", "
		<< glm::to_string(frustumAABB.mins());
	ASSERT_EQ(maxs, frustumAABB.maxs())
		<< glm::to_string(maxs) << ", "
		<< glm::to_string(frustumAABB.maxs());
	int n = 0;
	const int blockSize = glm::ceil(aabb.getWidthX() / slices);
	octree.visit(frustum, [&] (const glm::ivec3& currentMins, const glm::ivec3& currentMaxs) {
		const glm::ivec3 center = (currentMins + currentMaxs) / 2;
		for (int i = 0; i < center.length(); ++i) {
			EXPECT_EQ(center[i] % blockSize, slices) << glm::to_string(center);
		}
		++n;
		return true;
	}, glm::ivec3(blockSize));
	EXPECT_EQ(glm::pow(slices, glm::ivec3::length()), n);
}

TEST_F(OctreeTest, testOctreeComputeAABB1) {
	testAABB(glm::vec3(-4.0f), glm::vec3(6.0f), glm::ivec3(-32), glm::ivec3(32), 32);
}

TEST_F(OctreeTest, testOctreeComputeAABB2) {
	testAABB(glm::vec3(1.0f), glm::vec3(6.0f), glm::ivec3(0), glm::ivec3(32), 32);
}

TEST_F(OctreeTest, testOctreeComputeAABB3) {
	testAABB(
		glm::vec3(-34.0f, -12.0f, -1.0f),
		glm::vec3(19.0f, 17.0f, 33.0f),

		glm::ivec3(-64, -32, -32),
		glm::ivec3(64, 32, 96),

		32);
}

TEST_F(OctreeTest, testOctreeVisitOrthoFrustumNoPerfectMatch) {
	const glm::vec3 mins(-4.0f);
	const glm::vec3 maxs(6.0f);
	int n = 0;
	test(mins, maxs, n, glm::ivec3(-32), glm::ivec3(32), 32);
	EXPECT_EQ(n, 8);
}

TEST_F(OctreeTest, testOctreeVisitOrthoFrustumNoPerfectMatchJustOneField) {
	const glm::vec3 mins(1.0f);
	const glm::vec3 maxs(6.0f);
	int n = 0;
	test(mins, maxs, n, glm::ivec3(0), glm::ivec3(32), 32);
	EXPECT_EQ(n, 1);
}

TEST_F(OctreeTest, testOctreeVisitOrthoFrustumNoPerfectMatchBigAndUneven) {
	const glm::vec3 mins(-34.0f, -12.0f, -1.0f);
	const glm::vec3 maxs(19.0f, 17.0f, 33.0f);
	int n = 0;
	test(mins, maxs, n, glm::ivec3(-64, -32, -32), glm::ivec3(64, 32, 96), 32);
	EXPECT_LE(n, 4 * 2 * 4) << "More than the absolute maxs of the calculated max AABB";
	EXPECT_EQ(n, 18) << "Expected to get some bboxes not visited, because they are outside of the original frustum";
}

}
