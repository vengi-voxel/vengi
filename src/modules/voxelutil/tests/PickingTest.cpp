/**
 * @file
 */

#include "voxelutil/Picking.h"
#include "core/TraceScoped.h"
#include "app/tests/AbstractTest.h"
#include "math/tests/TestMathHelper.h"
#include "voxel/RawVolume.h"
#include "voxelutil/Raycast.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Axes.h"

namespace voxelutil {

class PickingTest : public app::AbstractTest {
protected:
	/**
	 * @brief This is just an implementation class for the pickVoxel function
	 *
	 * It makes note of the sort of empty voxel you're looking for in the constructor.
	 *
	 * Each time the operator() is called:
	 *  * if it's hit a voxel it sets up the result and returns false
	 *  * otherwise it preps the result for the next iteration and returns true
	 */
	template<typename VolumeType>
	class RaycastPickingFunctor {
	public:
		bool operator()(typename VolumeType::Sampler &sampler) {
			if (!_result.firstValidPosition && sampler.currentPositionValid()) {
				_result.firstPosition = sampler.position();
				_result.firstValidPosition = true;
			}

			if (!voxel::isAir(sampler.voxel().getMaterial())) {
				_result.didHit = true;
				_result.hitVoxel = sampler.position();
				return false;
			}
			if (sampler.currentPositionValid()) {
				_result.validPreviousPosition = true;
				_result.previousPosition = sampler.position();
			} else if (_result.firstValidPosition && !_result.firstInvalidPosition) {
				_result.firstInvalidPosition = true;
				_result.hitVoxel = sampler.position();
				return false;
			}
			return true;
		}
		PickResult _result;
	};

	/** Pick the first solid voxel along a vector */
	template<typename VolumeType>
	PickResult pickVoxel(const VolumeType *volData, const math::Vec3f &v3dStart, const math::Vec3f &v3dDirectionAndLength) {
		core_trace_scoped(pickVoxel);
		RaycastPickingFunctor<VolumeType> functor;
		raycastWithDirection(volData, v3dStart, v3dDirectionAndLength, functor);
		return functor._result;
	}
};

TEST_F(PickingTest, testPicking) {
	voxel::RawVolume v(voxel::Region::fromSize(11));
	ASSERT_TRUE(v.setVoxel(math::Vec3i(0), voxel::createVoxel(voxel::VoxelType::Generic, 0)));
	const math::Vec3f start(0.0f, 3.0f, 0.0f);
	const math::Vec3f direction = math::down() * 100.0f;
	const PickResult &result = pickVoxel(&v, start, direction);
	ASSERT_TRUE(result.didHit) << "Expected to hit the voxel at (0, 0, 0)";
	ASSERT_EQ(math::Vec3i(0), result.hitVoxel)
		<< "Expected to hit the voxel at (0, 0, 0) - but got " << result.hitVoxel.x << ", " << result.hitVoxel.y << ", "
		<< result.hitVoxel.z;
	ASSERT_TRUE(result.validPreviousPosition);
}

} // namespace voxelutil
