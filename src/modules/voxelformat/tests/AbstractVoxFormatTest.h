/**
 * @file
 */

#pragma once

#include "voxel/tests/AbstractVoxelTest.h"
#include "voxelformat/VoxFileFormat.h"

namespace voxel {

class AbstractVoxFormatTest: public AbstractVoxelTest {
protected:
	static const voxel::Voxel Empty;

	io::FilePtr open(const std::string_view filename, io::FileMode mode = io::FileMode::Read) {
		const io::FilePtr& file = io::filesystem()->open(std::string(filename), mode);
		return file;
	}

	voxel::RawVolume* load(const std::string_view filename, voxel::VoxFileFormat& format) {
		const io::FilePtr& file = open(filename);
		voxel::RawVolume* v = format.load(file);
		return v;
	}
};

}
