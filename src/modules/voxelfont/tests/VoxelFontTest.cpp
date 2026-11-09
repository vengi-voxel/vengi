/**
 * @file
 */

#include "voxelfont/VoxelFont.h"
#include "app/tests/AbstractTest.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Voxel.h"
#include <glm/vec3.hpp>

namespace voxelfont {

class VoxelFontTest : public app::AbstractTest {};

TEST_F(VoxelFontTest, RenderCharacterSetsVoxels) {
	VoxelFont font;
	ASSERT_TRUE(font.init("font.ttf"));
	voxel::Region region(0, 0, 0, 63, 63, 7);
	voxel::RawVolume volume(region);
	voxel::RawVolumeWrapper wrapper(&volume);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const int width = font.renderCharacter('A', 32, 2, glm::ivec3(8, 8, 0), wrapper, voxel);
	EXPECT_GT(width, 0);
	int count = 0;
	for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
		for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
			for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
				if (!voxel::isAir(volume.voxel(x, y, z).getMaterial())) {
					++count;
				}
			}
		}
	}
	EXPECT_GT(count, 0);
	int w = 0;
	int h = 0;
	font.dimensions("Hi", 32, w, h);
	EXPECT_GT(w, 0);
	EXPECT_GT(h, 0);
}

} // namespace voxelfont
