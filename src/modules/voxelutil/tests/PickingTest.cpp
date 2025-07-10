/**
 * @file
 */

#include "voxelutil/Picking.h"
#include "app/tests/AbstractTest.h"
#include "core/GLM.h"
#include "math/tests/TestMathHelper.h"
#include "voxel/RawVolume.h"

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
		RaycastPickingFunctor(const voxel::Voxel &emptyVoxelExample)
			: _emptyVoxelExample(emptyVoxelExample), _result() {
		}

		bool operator()(typename VolumeType::Sampler &sampler) {
			if (sampler.voxel() != _emptyVoxelExample) {
				// If we've hit something
				_result.didHit = true;
				_result.hitVoxel = sampler.position();
				return false;
			}

			if (sampler.currentPositionValid()) {
				_result.validPreviousPosition = true;
				_result.previousPosition = sampler.position();
			}
			return true;
		}
		const voxel::Voxel &_emptyVoxelExample;
		PickResult _result;
	};

	/** Pick the first solid voxel along a vector */
	template<typename VolumeType>
	PickResult pickVoxel(const VolumeType *volData, const glm::vec3 &v3dStart, const glm::vec3 &v3dDirectionAndLength,
						 const voxel::Voxel &emptyVoxelExample) {
		core_trace_scoped(pickVoxel);
		RaycastPickingFunctor<VolumeType> functor(emptyVoxelExample);
		raycastWithDirection(volData, v3dStart, v3dDirectionAndLength, functor);
		return functor._result;
	}
};

TEST_F(PickingTest, testPicking) {
	voxel::RawVolume v(voxel::Region(glm::ivec3(0), glm::ivec3(10)));
	ASSERT_TRUE(v.setVoxel(glm::ivec3(0), voxel::createVoxel(voxel::VoxelType::Generic, 0)));
	const glm::vec3 start(0.0f, 3.0f, 0.0f);
	const glm::vec3 direction = glm::down() * 100.0f;
	const PickResult &result = pickVoxel(&v, start, direction, voxel::Voxel());
	ASSERT_TRUE(result.didHit) << "Expected to hit the voxel at (0, 0, 0)";
	ASSERT_EQ(glm::ivec3(0), result.hitVoxel)
		<< "Expected to hit the voxel at (0, 0, 0) - but got " << result.hitVoxel.x << ", " << result.hitVoxel.y << ", "
		<< result.hitVoxel.z;
	ASSERT_TRUE(result.validPreviousPosition);
	ASSERT_EQ(glm::ivec3(0, 1, 0), result.previousPosition)
		<< "Expected to get a previous position of (0, 1, 0) - but got " << result.previousPosition.x << ", "
		<< result.previousPosition.y << ", " << result.previousPosition.z;
}

} // namespace voxelutil
