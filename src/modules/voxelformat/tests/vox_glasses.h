/**
 * @file Created with AbstractVoxFormatTest::dump from data/tests/vox_glasses.vox
 */

#pragma once

#include "core/SharedPtr.h"
#include "voxel/RawVolume.h"

struct glasses_0 {
	static core::SharedPtr<voxel::RawVolume> create() {
		glm::ivec3 mins(-7, 48, -2);
		glm::ivec3 maxs(6, 51, 6);
		voxel::Region region(mins, maxs);
		core::SharedPtr<voxel::RawVolume> v = core::make_shared<voxel::RawVolume>(region);
		v->setVoxel(-7, 50, -2, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(6, 50, -2, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(-7, 51, -2, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(6, 51, -2, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(-7, 51, -1, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(6, 51, -1, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(-7, 51, 0, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(6, 51, 0, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(-7, 51, 1, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(6, 51, 1, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(-7, 51, 2, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(6, 51, 2, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(-7, 51, 3, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(6, 51, 3, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(-7, 51, 4, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(6, 51, 4, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(-7, 51, 5, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(6, 51, 5, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(-4, 48, 6, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(3, 48, 6, voxel::createVoxel(voxel::VoxelType::Generic,  188));
		v->setVoxel(-5, 49, 6, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(-4, 49, 6, voxel::createVoxel(voxel::VoxelType::Generic,  188));
		v->setVoxel(-3, 49, 6, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(2, 49, 6, voxel::createVoxel(voxel::VoxelType::Generic,  188));
		v->setVoxel(3, 49, 6, voxel::createVoxel(voxel::VoxelType::Generic,  199));
		v->setVoxel(4, 49, 6, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(-6, 50, 6, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(-5, 50, 6, voxel::createVoxel(voxel::VoxelType::Generic,  188));
		v->setVoxel(-4, 50, 6, voxel::createVoxel(voxel::VoxelType::Generic,  199));
		v->setVoxel(-3, 50, 6, voxel::createVoxel(voxel::VoxelType::Generic,  188));
		v->setVoxel(-2, 50, 6, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(1, 50, 6, voxel::createVoxel(voxel::VoxelType::Generic,  188));
		v->setVoxel(2, 50, 6, voxel::createVoxel(voxel::VoxelType::Generic,  199));
		v->setVoxel(3, 50, 6, voxel::createVoxel(voxel::VoxelType::Generic,  188));
		v->setVoxel(4, 50, 6, voxel::createVoxel(voxel::VoxelType::Generic,  188));
		v->setVoxel(5, 50, 6, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(-7, 51, 6, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(-6, 51, 6, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(-5, 51, 6, voxel::createVoxel(voxel::VoxelType::Generic,  199));
		v->setVoxel(-4, 51, 6, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(-3, 51, 6, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(-2, 51, 6, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(-1, 51, 6, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(0, 51, 6, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(1, 51, 6, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(2, 51, 6, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(3, 51, 6, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(4, 51, 6, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(5, 51, 6, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		v->setVoxel(6, 51, 6, voxel::createVoxel(voxel::VoxelType::Generic,  193));
		return v;
	}
};
