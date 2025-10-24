/**
 * @file
 */

#include "voxel/CoordinateSystemVolume.h"
#include "VoxelPrinter.h"
#include "app/tests/AbstractTest.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"

namespace voxel {

class CoordinateSystemVolumeTest : public app::AbstractTest {};

TEST_F(CoordinateSystemVolumeTest, testSetVoxelMagicaVoxel) {
	const voxel::Region region(glm::ivec3(0), glm::ivec3(9));
	voxel::RawVolume volume(region);
	const math::CoordinateSystem srcSystem = math::CoordinateSystem::MagicaVoxel;
	CoordinateSystemVolume<voxel::RawVolume> csVolume(srcSystem, volume);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 42);
	EXPECT_TRUE(csVolume.setVoxel(1, 2, 3, voxel))
		<< "The conversion of the coordinates failed and the position was out of the region";
	const voxel::Voxel &v = volume.voxel(1, 6, 2);
	EXPECT_EQ(v.getColor(), voxel.getColor()) << volume;
}

TEST_F(CoordinateSystemVolumeTest, testSetVoxelDirectX) {
	const voxel::Region region(glm::ivec3(0), glm::ivec3(9));
	voxel::RawVolume volume(region);
	const math::CoordinateSystem srcSystem = math::CoordinateSystem::DirectX;
	CoordinateSystemVolume<voxel::RawVolume> csVolume(srcSystem, volume);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 42);
	EXPECT_TRUE(csVolume.setVoxel(1, 2, 3, voxel))
		<< "The conversion of the coordinates failed and the position was out of the region";
	const voxel::Voxel &v = volume.voxel(1, 2, 6);
	EXPECT_EQ(v.getColor(), voxel.getColor()) << volume;
}

TEST_F(CoordinateSystemVolumeTest, testSetVoxelOpenGL) {
	const voxel::Region region(glm::ivec3(0), glm::ivec3(9));
	voxel::RawVolume volume(region);
	const math::CoordinateSystem srcSystem = math::CoordinateSystem::OpenGL;
	CoordinateSystemVolume<voxel::RawVolume> csVolume(srcSystem, volume);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 42);
	EXPECT_TRUE(csVolume.setVoxel(1, 2, 3, voxel))
		<< "The conversion of the coordinates failed and the position was out of the region";
	const voxel::Voxel &v = volume.voxel(1, 2, 3);
	EXPECT_EQ(v.getColor(), voxel.getColor()) << volume;
}

TEST_F(CoordinateSystemVolumeTest, testSetVoxelOutOfBounds) {
	const voxel::Region region(glm::ivec3(0), glm::ivec3(9));
	voxel::RawVolume volume(region);
	const math::CoordinateSystem srcSystem = math::CoordinateSystem::MagicaVoxel;
	CoordinateSystemVolume<voxel::RawVolume> csVolume(srcSystem, volume);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 42);
	EXPECT_FALSE(csVolume.setVoxel(1, 2, 10, voxel));
	EXPECT_FALSE(csVolume.setVoxel(1, 10, 2, voxel));
	EXPECT_FALSE(csVolume.setVoxel(10, 1, 2, voxel));
}

TEST_F(CoordinateSystemVolumeTest, testSetVoxelOnLowerBounds) {
	const voxel::Region region(glm::ivec3(0), glm::ivec3(9));
	voxel::RawVolume volume(region);
	const math::CoordinateSystem srcSystem = math::CoordinateSystem::MagicaVoxel;
	CoordinateSystemVolume<voxel::RawVolume> csVolume(srcSystem, volume);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 42);
	EXPECT_TRUE(csVolume.setVoxel(0, 0, 0, voxel));
	const voxel::Voxel &v = volume.voxel(0, 9, 0);
	EXPECT_EQ(v.getColor(), voxel.getColor()) << volume;
}

TEST_F(CoordinateSystemVolumeTest, testSetVoxelOnUpperBounds) {
	const voxel::Region region(glm::ivec3(0), glm::ivec3(9));
	voxel::RawVolume volume(region);
	const math::CoordinateSystem srcSystem = math::CoordinateSystem::MagicaVoxel;
	CoordinateSystemVolume<voxel::RawVolume> csVolume(srcSystem, volume);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 42);
	EXPECT_TRUE(csVolume.setVoxel(9, 9, 9, voxel));
	const voxel::Voxel &v = volume.voxel(9, 0, 9);
	EXPECT_EQ(v.getColor(), voxel.getColor()) << volume;
}

} // namespace voxel
