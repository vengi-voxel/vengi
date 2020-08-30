/**
 * @file
 */

#pragma once

#include "voxel/tests/AbstractVoxelTest.h"
#include "voxelformat/VoxFileFormat.h"
#include "io/Filesystem.h"

namespace voxel {

class AbstractVoxFormatTest: public AbstractVoxelTest {
protected:
	static const voxel::Voxel Empty;

	io::FilePtr open(const core::String& filename, io::FileMode mode = io::FileMode::Read) {
		const io::FilePtr& file = io::filesystem()->open(core::String(filename), mode);
		return file;
	}

	voxel::RawVolume* load(const core::String& filename, voxel::VoxFileFormat& format) {
		const io::FilePtr& file = open(filename);
		voxel::RawVolume* v = format.load(file);
		return v;
	}
};

}
