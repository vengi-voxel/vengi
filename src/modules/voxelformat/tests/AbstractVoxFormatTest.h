/**
 * @file
 */

#pragma once

#include "io/FileStream.h"
#include "voxel/tests/AbstractVoxelTest.h"
#include "voxelformat/Format.h"
#include "io/Filesystem.h"

namespace voxel {

class AbstractVoxFormatTest: public AbstractVoxelTest {
protected:
	static const voxel::Voxel Empty;

	void testRGB(RawVolume* volume) {
		EXPECT_EQ(VoxelType::Generic, volume->voxel( 0,  0,  0).getMaterial());
		EXPECT_EQ(VoxelType::Generic, volume->voxel(31,  0,  0).getMaterial());
		EXPECT_EQ(VoxelType::Generic, volume->voxel(31,  0, 31).getMaterial());
		EXPECT_EQ(VoxelType::Generic, volume->voxel( 0,  0, 31).getMaterial());

		EXPECT_EQ(VoxelType::Generic, volume->voxel( 0, 31,  0).getMaterial());
		EXPECT_EQ(VoxelType::Generic, volume->voxel(31, 31,  0).getMaterial());
		EXPECT_EQ(VoxelType::Generic, volume->voxel(31, 31, 31).getMaterial());
		EXPECT_EQ(VoxelType::Generic, volume->voxel( 0, 31, 31).getMaterial());

		EXPECT_EQ(VoxelType::Generic, volume->voxel( 9,  0,  4).getMaterial());
		EXPECT_EQ(VoxelType::Generic, volume->voxel( 9,  0, 12).getMaterial());
		EXPECT_EQ(VoxelType::Generic, volume->voxel( 9,  0, 19).getMaterial());

		EXPECT_EQ(  0, volume->voxel( 0,  0,  0).getColor());
		EXPECT_EQ(  0, volume->voxel(31,  0,  0).getColor());
		EXPECT_EQ(  0, volume->voxel(31,  0, 31).getColor());
		EXPECT_EQ(  0, volume->voxel( 0,  0, 31).getColor());

		EXPECT_EQ(  1, volume->voxel( 0, 31,  0).getColor());
		EXPECT_EQ(  1, volume->voxel(31, 31,  0).getColor());
		EXPECT_EQ(  1, volume->voxel(31, 31, 31).getColor());
		EXPECT_EQ(  1, volume->voxel( 0, 31, 31).getColor());

		EXPECT_EQ( 37, volume->voxel( 9,  0,  4).getColor());
		EXPECT_EQ(149, volume->voxel( 9,  0, 12).getColor());
		EXPECT_EQ(197, volume->voxel( 9,  0, 19).getColor());
	}

	void testSaveMultipleLayers(const core::String& filename, voxel::Format* format);

	void testSaveLoadVoxel(const core::String& filename, voxel::Format* format, int mins = 0, int maxs = 1);

	io::FilePtr open(const core::String& filename, io::FileMode mode = io::FileMode::Read) {
		const io::FilePtr& file = io::filesystem()->open(core::String(filename), mode);
		return file;
	}

	voxel::RawVolume* load(const core::String& filename, voxel::Format& format) {
		const io::FilePtr& file = open(filename);
		if (!file->validHandle()) {
			return nullptr;
		}
		io::FileStream stream(file.get());
		voxel::RawVolume* v = format.load(filename, stream);
		return v;
	}
};

}
