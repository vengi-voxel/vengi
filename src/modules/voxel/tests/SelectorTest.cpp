/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "voxel/Selector.h"
#include "voxel/polyvox/RawVolume.h"
#include "voxel/polyvox/Region.h"
#include "voxel/polyvox/Voxel.h"

namespace voxel {

class SelectorTest: public core::AbstractTest {
private:
	using Super = core::AbstractTest;
protected:
	voxel::Region _region{0, 31};
	voxel::RawVolume* _volume;
	Selector _selector;
public:
	void SetUp() override {
		Super::SetUp();
		_volume = new voxel::RawVolume(_region);
		constexpr voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
		for (int x = 0; x <= 31; ++x) {
			for (int y = 0; y <= 31; ++y) {
				_volume->setVoxel(x, y, 0, voxel);
			}
		}
	}

	void TearDown() override {
		delete _volume;
		Super::TearDown();
	}
};

TEST_F(SelectorTest, testWalk) {
	voxel::RawVolume::Sampler sampler(_volume);
	int cnt = 0;
	SelectorCallback foo = [&] (const voxel::RawVolume::Sampler& sampler, voxel::FaceNames name) {
		++cnt;
		return true;
	};
	_selector.walk(sampler, foo);
	ASSERT_EQ(32*32*32, cnt) << "Unexpected amount of voxels visited. Expected to visit the whole volume.";
}

TEST_F(SelectorTest, testSkipAir) {
	voxel::RawVolume::Sampler sampler(_volume);
	int cnt = 0;
	SelectorCallback foo = [&] (const voxel::RawVolume::Sampler& sampler, voxel::FaceNames name) {
		++cnt;
		if (voxel::isAir(sampler.voxel().getMaterial())) {
			return false;
		}
		return true;
	};
	_selector.walk(sampler, foo);
	ASSERT_EQ(32*32*2, cnt) << "Unexpected amount of voxels visited. Expected to visit all solid voxels and all directly connected empty voxels.";
}

}
