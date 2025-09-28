/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "core/Bits.h"
#include "core/Enum.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

#include <glm/geometric.hpp>

namespace voxel {

inline ::std::ostream& operator<<(::std::ostream& os, const voxel::FaceBits& facebits) {
	return os << "bits[" << core::toBitString(core::enumVal(facebits)) << "]";
}

class FaceTest: public app::AbstractTest {
};

TEST_F(FaceTest, testVisibility) {
	const voxel::Region region(0, 31);
	const voxel::Voxel voxel = voxel::createVoxel(VoxelType::Generic, 1);
	voxel::RawVolume volume(region);
	for (int x = 0; x < 3; ++x) {
		for (int y = 0; y < 3; ++y) {
			for (int z = 0; z < 3; ++z) {
				volume.setVoxel(x, y, z, voxel);
			}
		}
	}


	EXPECT_EQ(FaceBits::NegativeX | FaceBits::NegativeY | FaceBits::NegativeZ, visibleFaces(volume, 0, 0, 0));
	EXPECT_EQ(FaceBits::None, visibleFaces(volume, 1, 1, 1));
	EXPECT_EQ(FaceBits::PositiveX | FaceBits::PositiveY | FaceBits::PositiveZ, visibleFaces(volume, 2, 2, 2));
	EXPECT_EQ(FaceBits::PositiveX | FaceBits::PositiveZ, visibleFaces(volume, 2, 1, 2));
}

}
