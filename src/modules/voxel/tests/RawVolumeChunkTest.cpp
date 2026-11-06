/**
 * @file
 */

#include "AbstractVoxelTest.h"
#include "core/ArrayLength.h"
#include "core/StandardLib.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeMoveWrapper.h"
#include "voxel/RawVolumeWrapper.h"

namespace voxel {

class RawVolumeChunkTest : public AbstractVoxelTest {
protected:
	const Region largeRegion{-40, -20, -60, 216, 236, 196};
};

TEST_F(RawVolumeChunkTest, LazyReadWriteCopyAndTranslate) {
	RawVolume volume(largeRegion);
	const size_t directoryBytes = volume.allocatedBytes();
	EXPECT_LT(directoryBytes, RawVolume::size(largeRegion) / 100);
	EXPECT_TRUE(volume.isEmpty(largeRegion));
	const Voxel empty;
	const Voxel value(VoxelType::Generic, 41, 9, FlagOutline, 7);
	const Voxel border(VoxelType::Transparent, 2);
	volume.setBorderValue(border);
	EXPECT_EQ(border, volume.voxel(-41, -20, -60));
	const glm::ivec3 positions[] = {{-40, -20, -60}, {-9, 11, -29}, {-8, 12, -28}, {216, 236, 196}};
	for (const auto &pos : positions) {
		EXPECT_EQ(empty, volume.voxel(pos));
		EXPECT_TRUE(volume.setVoxel(largeRegion.index(pos), value));
		EXPECT_FALSE(volume.setVoxel(pos, value));
		EXPECT_EQ(value, volume.voxel(pos));
	}
	EXPECT_EQ(nullptr, volume.linearVoxels());
	EXPECT_LT(volume.allocatedBytes(), RawVolume::size(largeRegion) / 10);
	RawVolume copy(volume);
	EXPECT_EQ(volume.allocatedBytes(), copy.allocatedBytes());
	copy.clear();
	EXPECT_EQ(value, volume.voxel(positions[0]));
	EXPECT_EQ(empty, copy.voxel(positions[0]));
	EXPECT_TRUE(copy.isEmpty(largeRegion));
	RawVolume moved(core::move(volume));
	const glm::ivec3 delta(17, -3, 9);
	moved.translate(delta);
	for (const auto &pos : positions) {
		EXPECT_EQ(value, moved.voxel(pos + delta));
	}
	EXPECT_EQ(border, moved.voxel(largeRegion.getLowerCorner() + delta - 1));
	EXPECT_FALSE(moved.setVoxel((int64_t)-1, value));
}

TEST_F(RawVolumeChunkTest, SamplerBoundariesAndWrappersStayChunked) {
	RawVolume volume(largeRegion);
	RawVolume::Sampler sampler(volume);
	const glm::ivec3 boundary = largeRegion.getLowerCorner() + 32;
	const Voxel value(VoxelType::Generic, 11);
	for (int z = -1; z <= 1; ++z) {
		for (int y = -1; y <= 1; ++y) {
			for (int x = -1; x <= 1; ++x) {
				ASSERT_TRUE(sampler.setPosition(boundary + glm::ivec3(x, y, z)));
				EXPECT_TRUE(sampler.setVoxel(value));
				EXPECT_EQ(value, sampler.voxel());
			}
		}
	}
	ASSERT_TRUE(sampler.setPosition(boundary));
	EXPECT_EQ(value, sampler.peekVoxel0px0py0pz());
	EXPECT_EQ(value, sampler.peekVoxel1nx1ny1nz());
	EXPECT_EQ(value, sampler.peekVoxel1px1py1pz());
	EXPECT_EQ(value, sampler.peekVoxel0px1ny0pz());
	EXPECT_EQ(value, sampler.peekVoxel1px0py0pz());
	sampler.moveNegativeX(32);
	EXPECT_EQ(Voxel(), sampler.voxel());
	sampler.movePositiveX(32);
	EXPECT_EQ(value, sampler.voxel());
	sampler.movePositiveY();
	EXPECT_EQ(value, sampler.voxel());
	sampler.moveNegativeY();
	sampler.movePositiveZ();
	EXPECT_EQ(value, sampler.voxel());
	sampler.moveNegativeZ();
	ASSERT_TRUE(sampler.setPosition(largeRegion.getUpperCorner()));
	sampler.movePositiveX();
	EXPECT_FALSE(sampler.currentPositionValid());
	EXPECT_FALSE(sampler.setVoxel(value));
	sampler.moveNegativeX();
	EXPECT_TRUE(sampler.currentPositionValid());
	RawVolumeWrapper wrapper(&volume);
	{
		RawVolumeWrapper::Sampler wrapped(wrapper);
		ASSERT_TRUE(wrapped.setPosition(boundary + 2));
		EXPECT_TRUE(wrapped.setVoxel(value));
		EXPECT_EQ(value, wrapped.voxel());
	}
	EXPECT_TRUE(wrapper.dirtyRegion().containsPoint(boundary + 2));
	RawVolumeMoveWrapper moveWrapper(&volume);
	RawVolumeMoveWrapper::Sampler wrapping(moveWrapper);
	wrapping.setPosition(glm::ivec3(257));
	EXPECT_TRUE(wrapping.setVoxel(value));
	EXPECT_EQ(value, volume.voxel(largeRegion.getLowerCorner()));
	EXPECT_EQ(nullptr, volume.linearVoxels());
}

TEST_F(RawVolumeChunkTest, FlagsAndPartialCopyMatchDenseStorage) {
	RawVolume volume(largeRegion);
	RawVolume dense(largeRegion);
	ASSERT_NE(nullptr, dense.voxels());
	const glm::ivec3 pos = largeRegion.getLowerCorner() + 31;
	const Region edited(pos, pos + 3);
	const Voxel value(VoxelType::Generic, 12, 5, 0, 6);
	volume.setVoxel(pos, value);
	dense.setVoxel(pos, value);
	volume.setFlags(edited, FlagOutline);
	dense.setFlags(edited, FlagOutline);
	EXPECT_EQ(dense.regionForFlag(FlagOutline), volume.regionForFlag(FlagOutline));
	EXPECT_EQ(dense.hasFlags(largeRegion, FlagOutline), volume.hasFlags(largeRegion, FlagOutline));
	EXPECT_EQ(dense.isEmpty(largeRegion), volume.isEmpty(largeRegion));
	volume.toggleFlags(Region(pos + 1, pos + 2), FlagOutline);
	dense.toggleFlags(Region(pos + 1, pos + 2), FlagOutline);
	volume.removeFlags(Region(pos + 2, pos + 4), FlagOutline);
	dense.removeFlags(Region(pos + 2, pos + 4), FlagOutline);
	for (int z = pos.z - 1; z <= pos.z + 4; ++z) {
		for (int y = pos.y - 1; y <= pos.y + 4; ++y) {
			for (int x = pos.x - 1; x <= pos.x + 4; ++x) {
				EXPECT_EQ(dense.voxel(x, y, z), volume.voxel(x, y, z));
			}
		}
	}
	bool onlyAir = true;
	RawVolume cropped(volume, edited, &onlyAir);
	EXPECT_FALSE(onlyAir);
	RawVolume denseCrop(dense, edited);
	EXPECT_EQ(cropped.region(), denseCrop.region());
	EXPECT_EQ(0, core_memcmp(cropped.data(), denseCrop.data(), RawVolume::size(edited)));
	EXPECT_EQ(nullptr, volume.linearVoxels());
	volume.removeFlags(largeRegion, FlagOutline);
	EXPECT_FALSE(volume.hasFlags(largeRegion, FlagOutline));
	EXPECT_EQ(Region::InvalidRegion, volume.regionForFlag(FlagOutline));
}

TEST_F(RawVolumeChunkTest, UniformFillAndContiguousCompatibility) {
	RawVolume volume(largeRegion);
	const size_t directoryBytes = volume.allocatedBytes();
	const Voxel value(VoxelType::Generic, 12, 17, FlagOutline, 8);
	volume.fill(value);
	EXPECT_EQ(directoryBytes, volume.allocatedBytes());
	EXPECT_EQ(value, volume.voxel(largeRegion.getUpperCorner()));
	Voxel *copy = volume.copyVoxels();
	ASSERT_NE(nullptr, copy);
	EXPECT_EQ(value, copy[0]);
	EXPECT_EQ(value, copy[largeRegion.voxels() - 1]);
	core_free(copy);
	EXPECT_EQ(nullptr, volume.linearVoxels());
	Voxel *data = volume.voxels();
	ASSERT_NE(nullptr, data);
	EXPECT_EQ(value, data[largeRegion.voxels() - 1]);
	core_memset(data + largeRegion.voxels() - 1, 0, sizeof(Voxel));
	EXPECT_EQ(Voxel(), volume.voxel(largeRegion.getUpperCorner()));
	EXPECT_EQ(RawVolume::size(largeRegion), volume.allocatedBytes());
	volume.clear();
	EXPECT_TRUE(volume.isEmpty(largeRegion));
}


TEST_F(RawVolumeChunkTest, AllSamplerNeighboursAtChunkBoundaries) {
	RawVolume volume(largeRegion);
	const glm::ivec3 boundary = largeRegion.getLowerCorner() + 32;
	for (int z = -2; z <= 2; ++z) {
		for (int y = -2; y <= 2; ++y) {
			for (int x = -2; x <= 2; ++x) {
				volume.setVoxel(boundary + glm::ivec3(x, y, z), Voxel(VoxelType::Generic,
					(uint8_t)(1 + (x + 2) + 5 * (y + 2) + 25 * (z + 2))));
			}
		}
	}
	struct Neighbour {
		glm::ivec3 offset;
		const Voxel &(RawVolume::Sampler::*peek)() const;
	};
	const Neighbour neighbours[] = {
		{{-1, -1, -1}, &RawVolume::Sampler::peekVoxel1nx1ny1nz},
		{{0, -1, -1}, &RawVolume::Sampler::peekVoxel0px1ny1nz},
		{{1, -1, -1}, &RawVolume::Sampler::peekVoxel1px1ny1nz},
		{{-1, 0, -1}, &RawVolume::Sampler::peekVoxel1nx0py1nz},
		{{0, 0, -1}, &RawVolume::Sampler::peekVoxel0px0py1nz},
		{{1, 0, -1}, &RawVolume::Sampler::peekVoxel1px0py1nz},
		{{-1, 1, -1}, &RawVolume::Sampler::peekVoxel1nx1py1nz},
		{{0, 1, -1}, &RawVolume::Sampler::peekVoxel0px1py1nz},
		{{1, 1, -1}, &RawVolume::Sampler::peekVoxel1px1py1nz},
		{{-1, -1, 0}, &RawVolume::Sampler::peekVoxel1nx1ny0pz},
		{{0, -1, 0}, &RawVolume::Sampler::peekVoxel0px1ny0pz},
		{{1, -1, 0}, &RawVolume::Sampler::peekVoxel1px1ny0pz},
		{{-1, 0, 0}, &RawVolume::Sampler::peekVoxel1nx0py0pz},
		{{0, 0, 0}, &RawVolume::Sampler::peekVoxel0px0py0pz},
		{{1, 0, 0}, &RawVolume::Sampler::peekVoxel1px0py0pz},
		{{-1, 1, 0}, &RawVolume::Sampler::peekVoxel1nx1py0pz},
		{{0, 1, 0}, &RawVolume::Sampler::peekVoxel0px1py0pz},
		{{1, 1, 0}, &RawVolume::Sampler::peekVoxel1px1py0pz},
		{{-1, -1, 1}, &RawVolume::Sampler::peekVoxel1nx1ny1pz},
		{{0, -1, 1}, &RawVolume::Sampler::peekVoxel0px1ny1pz},
		{{1, -1, 1}, &RawVolume::Sampler::peekVoxel1px1ny1pz},
		{{-1, 0, 1}, &RawVolume::Sampler::peekVoxel1nx0py1pz},
		{{0, 0, 1}, &RawVolume::Sampler::peekVoxel0px0py1pz},
		{{1, 0, 1}, &RawVolume::Sampler::peekVoxel1px0py1pz},
		{{-1, 1, 1}, &RawVolume::Sampler::peekVoxel1nx1py1pz},
		{{0, 1, 1}, &RawVolume::Sampler::peekVoxel0px1py1pz},
		{{1, 1, 1}, &RawVolume::Sampler::peekVoxel1px1py1pz},
	};
	RawVolume::Sampler sampler(volume);
	for (int shift = -1; shift <= 1; ++shift) {
		const glm::ivec3 pos = boundary + shift;
		ASSERT_TRUE(sampler.setPosition(pos));
		for (const auto &neighbour : neighbours) {
			EXPECT_EQ(volume.voxel(pos + neighbour.offset), (sampler.*neighbour.peek)());
		}
	}
	RawVolume missing(largeRegion);
	RawVolume::Sampler reader(missing);
	ASSERT_TRUE(reader.setPosition(boundary));
	EXPECT_EQ(Voxel(), reader.voxel());
	missing.setVoxel(boundary, Voxel(VoxelType::Generic, 99));
	EXPECT_EQ(99, reader.voxel().getColor());
	reader.movePositiveX();
	EXPECT_EQ(Voxel(), reader.voxel());
	reader.moveNegativeX();
	EXPECT_EQ(99, reader.voxel().getColor());
	reader.movePositiveX(32);
	EXPECT_EQ(Voxel(), reader.voxel());
	missing.setVoxel(boundary + glm::ivec3(32, 0, 0), Voxel(VoxelType::Generic, 98));
	EXPECT_EQ(98, reader.voxel().getColor());
	EXPECT_EQ(nullptr, missing.linearVoxels());
}

TEST_F(RawVolumeChunkTest, FillBehindPlaneMatchesDenseStorage) {
	RawVolume volume(largeRegion);
	RawVolume dense(largeRegion);
	ASSERT_NE(nullptr, dense.voxels());
	const Voxel value(VoxelType::Generic, 17);
	for (math::Axis axis : {math::Axis::X, math::Axis::Y, math::Axis::Z}) {
		volume.clear();
		dense.clear();
		const int d = math::getIndexForAxis(axis);
		const int a = math::getIndexForAxis(math::getPlaneAxisA(axis));
		const int b = math::getIndexForAxis(math::getPlaneAxisB(axis));
		glm::ivec3 pos = largeRegion.getLowerCorner() + 33;
		volume.setVoxel(pos, value);
		dense.setVoxel(pos, value);
		Voxel cells[16];
		Voxel expected[16];
		cells[0] = value;
		expected[0] = value;
		volume.fillBehindPlane(axis, pos[d] + 1, pos[a] - 2, pos[a] + 1, pos[b] - 2, pos[b] + 1, cells);
		dense.fillBehindPlane(axis, pos[d] + 1, pos[a] - 2, pos[a] + 1, pos[b] - 2, pos[b] + 1, expected);
		for (int i = 0; i < 16; ++i) {
			EXPECT_EQ(expected[i], cells[i]);
		}
		EXPECT_EQ(nullptr, volume.linearVoxels());
	}
}

TEST_F(RawVolumeChunkTest, SamplerMovesAcrossBlocksAndInvalidPositions) {
	for (bool materialize : {false, true}) {
		RawVolume volume(largeRegion);
		if (materialize) {
			ASSERT_NE(nullptr, volume.voxels());
		}
		const int steps[] = {0, 1, 30, 1, 1, 32, 64, 128, 1};
		for (int axis = 0; axis < 3; ++axis) {
			glm::ivec3 expected = largeRegion.getLowerCorner();
			RawVolume::Sampler sampler(volume);
			ASSERT_TRUE(sampler.setPosition(expected));
			for (int step : steps) {
				expected[axis] += step;
				if (largeRegion.containsPoint(expected)) {
					volume.setVoxel(expected, Voxel(VoxelType::Generic, (uint8_t)(expected[axis] + 100)));
				}
				sampler.movePositive((math::Axis)(1 << axis), step);
				EXPECT_EQ(expected, sampler.position());
				EXPECT_EQ(largeRegion.containsPoint(expected), sampler.currentPositionValid());
				EXPECT_EQ(volume.voxel(expected), sampler.voxel());
			}
			for (int i = lengthof(steps) - 1; i >= 0; --i) {
				expected[axis] -= steps[i];
				sampler.moveNegative((math::Axis)(1 << axis), steps[i]);
				EXPECT_EQ(expected, sampler.position());
				EXPECT_EQ(largeRegion.containsPoint(expected), sampler.currentPositionValid());
				EXPECT_EQ(volume.voxel(expected), sampler.voxel());
			}
		}
		RawVolume::Sampler sampler(volume);
		sampler.setPosition(largeRegion.getUpperCorner());
		sampler.movePositiveX();
		sampler.movePositiveY();
		sampler.moveNegativeX();
		EXPECT_FALSE(sampler.currentPositionValid());
		sampler.moveNegativeY();
		EXPECT_TRUE(sampler.currentPositionValid());
		EXPECT_EQ(largeRegion.getUpperCorner(), sampler.position());
		EXPECT_EQ(materialize, volume.linearVoxels() != nullptr);
	}
}

TEST_F(RawVolumeChunkTest, SamplerRespectsClippedWrapperBounds) {
	RawVolume volume(largeRegion);
	volume.fill(Voxel(VoxelType::Generic, 9));
	const Region view(largeRegion.getLowerCorner() + 5, largeRegion.getLowerCorner() + 70);
	RawVolumeWrapper wrapper(&volume, view);
	for (int axis = 0; axis < 3; ++axis) {
		glm::ivec3 marked = view.getLowerCorner();
		marked[axis] = largeRegion.getLowerCorner()[axis] + 32;
		volume.setVoxel(marked, Voxel(VoxelType::Generic, (uint8_t)(17 + axis)));
		RawVolumeWrapper::Sampler sampler(wrapper);
		glm::ivec3 pos = view.getLowerCorner();
		ASSERT_TRUE(sampler.setPosition(pos));
		for (int step : {0, 1, 26, 1, 5, 32, 1}) {
			pos[axis] += step;
			sampler.movePositive((math::Axis)(1 << axis), step);
			EXPECT_EQ(pos, sampler.position());
			EXPECT_EQ(view.containsPoint(pos), sampler.currentPositionValid());
			EXPECT_EQ(volume.voxel(pos), sampler.voxel());
		}
	}
	EXPECT_EQ(nullptr, volume.linearVoxels());
}

} // namespace voxel
