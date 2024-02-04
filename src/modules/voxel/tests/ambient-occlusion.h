/**
 * @file
 *
 * Created with AbstractVoxFormatTest::dump from data/tests/ambient-occlusion.vengi
 */

#pragma once

#include "core/SharedPtr.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

struct ambientocclusion {
	static core::SharedPtr<voxel::RawVolume> create() {
		glm::ivec3 mins(13, 14, 2);
		glm::ivec3 maxs(17, 18, 6);
		voxel::Region region(mins, maxs);
		core::SharedPtr<voxel::RawVolume> v = core::make_shared<voxel::RawVolume>(region);
		v->setVoxel(15, 16, 2, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(14, 15, 3, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(15, 15, 3, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(16, 15, 3, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(14, 16, 3, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(15, 16, 3, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(16, 16, 3, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(14, 17, 3, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(15, 17, 3, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(16, 17, 3, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(15, 14, 4, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(14, 15, 4, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(15, 15, 4, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(16, 15, 4, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(13, 16, 4, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(14, 16, 4, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(15, 16, 4, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(16, 16, 4, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(17, 16, 4, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(14, 17, 4, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(15, 17, 4, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(16, 17, 4, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(15, 18, 4, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(14, 15, 5, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(15, 15, 5, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(16, 15, 5, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(14, 16, 5, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		v->setVoxel(15, 16, 5, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(16, 16, 5, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(14, 17, 5, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(15, 17, 5, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(16, 17, 5, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(15, 16, 6, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		return v;
	}

	// https://github.com/vengi-voxel/vengi/issues/388
	static core::SharedPtr<voxel::RawVolume> createIssue338() {
		glm::ivec3 mins(0, 0, 0);
		glm::ivec3 maxs(1, 0, 1);
		voxel::Region region(mins, maxs);
		core::SharedPtr<voxel::RawVolume> v = core::make_shared<voxel::RawVolume>(region);
		v->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 244));
		v->setVoxel(1, 0, 1, voxel::createVoxel(voxel::VoxelType::Generic, 244));
		return v;
	}
};
