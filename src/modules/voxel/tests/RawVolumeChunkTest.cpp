/**
 * @file
 */

#include "AbstractVoxelTest.h"
#include "core/ArrayLength.h"
#include "core/StandardLib.h"
#include "core/ScopedPtr.h"
#include "core/concurrent/Atomic.h"
#include "core/concurrent/Thread.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeMoveWrapper.h"
#include "voxel/RawVolumeWrapper.h"
#include <limits.h>

namespace voxel {

class RawVolumeChunkTest : public AbstractVoxelTest {
protected:
	const Region largeRegion{-40, -20, -60, 216, 236, 196};
};

// No application workers may run while the SDL memory callbacks are replaced.
TEST(RawVolumeAllocationFailureTest, CopyConstructorsDoNotReturnPartialVolumes) {
	const int previous = RawVolume::chunkedStorageThreshold();
	RawVolume::setChunkedStorageThreshold(1);
	RawVolume source(Region(0, 64));
	RawVolume::setChunkedStorageThreshold(previous);
	const Voxel value(VoxelType::Generic, 7);
	const Voxel second(VoxelType::Generic, 9);
	source.setVoxel(0, 0, 0, value);
	source.setVoxel(32, 32, 32, second);
	ASSERT_TRUE(source.valid());
	const size_t allocated = source.allocatedBytes();
	core::Buffer<Region> regions;
	regions.push_back(Region(0, 1));
	regions.push_back(Region(32, 33));
	static SDL_malloc_func originalMalloc;
	static int remainingChunks;
	SDL_calloc_func originalCalloc;
	SDL_realloc_func originalRealloc;
	SDL_free_func originalFree;
	SDL_GetMemoryFunctions(&originalMalloc, &originalCalloc, &originalRealloc, &originalFree);
	const auto failingMalloc = [](size_t bytes) -> void * {
		if (bytes == 32u * 32u * 32u * sizeof(Voxel) && remainingChunks >= 0 && remainingChunks-- == 0) {
			return nullptr;
		}
		return originalMalloc(bytes);
	};
	for (int mode = 0; mode < 3; ++mode) {
		for (int failure = 0; failure < 2; ++failure) {
			remainingChunks = failure;
			bool onlyAir = true;
			RawVolume::setChunkedStorageThreshold(1);
			const bool installed = SDL_SetMemoryFunctions(failingMalloc, originalCalloc, originalRealloc, originalFree);
			core::ScopedPtr<RawVolume> copy(mode == 0 ? new RawVolume(source) : mode == 1
				? new RawVolume(source, Region(0, 63), &onlyAir) : new RawVolume(source, regions));
			const bool restored = SDL_SetMemoryFunctions(originalMalloc, originalCalloc, originalRealloc, originalFree);
			RawVolume::setChunkedStorageThreshold(previous);
			ASSERT_TRUE(installed);
			ASSERT_TRUE(restored);
			EXPECT_EQ(-1, remainingChunks);
			EXPECT_FALSE(copy->valid());
			EXPECT_FALSE(copy->move(glm::ivec3(0)));
			EXPECT_FALSE(copy->move(glm::ivec3(1)));
			EXPECT_EQ(nullptr, copy->voxels());
			EXPECT_EQ(nullptr, copy->data());
			EXPECT_EQ(nullptr, copy->copyVoxels());
			EXPECT_EQ(0u, copy->allocatedBytes());
			if (mode == 1) {
				EXPECT_FALSE(onlyAir);
			}
			EXPECT_EQ(value, source.voxel(0, 0, 0));
			EXPECT_EQ(second, source.voxel(32, 32, 32));
			EXPECT_EQ(allocated, source.allocatedBytes());
		}
	}
}


// No application workers may run while the SDL memory callbacks are replaced.
TEST(RawVolumeAllocationFailureTest, WrappersReportFailedWrites) {
	const int previous = RawVolume::chunkedStorageThreshold();
	static SDL_malloc_func originalMalloc;
	static bool failChunk;
	SDL_calloc_func originalCalloc;
	SDL_realloc_func originalRealloc;
	SDL_free_func originalFree;
	SDL_GetMemoryFunctions(&originalMalloc, &originalCalloc, &originalRealloc, &originalFree);
	const auto failingMalloc = [](size_t bytes) -> void * {
		if (failChunk && bytes == 32u * 32u * 32u * sizeof(Voxel)) {
			failChunk = false;
			return nullptr;
		}
		return originalMalloc(bytes);
	};
	const Voxel value(VoxelType::Generic, 7);
	for (int mode = 0; mode < 6; ++mode) {
		SCOPED_TRACE(mode);
		RawVolume::setChunkedStorageThreshold(1);
		RawVolume volume(Region(0, 64));
		RawVolume::setChunkedStorageThreshold(previous);
		RawVolumeWrapper wrapper(&volume);
		RawVolumeMoveWrapper moving(&volume);
		RawVolumeWrapper::Sampler sampler(wrapper);
		RawVolumeMoveWrapper::Sampler movingSampler(moving);
		const bool outside = mode == 2 || mode == 4;
		const glm::ivec3 requested(outside ? -1 : 0);
		const glm::ivec3 actual = outside ? volume.region().moveInto(requested.x, requested.y, requested.z) : requested;
		sampler.setPosition(0, 0, 0);
		movingSampler.setPosition(requested);
		const auto write = [&](const Voxel &voxel) {
			if (mode == 0) {
				return wrapper.setVoxel(requested, voxel);
			}
			if (mode == 1 || mode == 2) {
				return moving.setVoxel(requested, voxel);
			}
			return mode == 5 ? sampler.setVoxel(voxel) : movingSampler.setVoxel(voxel);
		};
		const size_t before = volume.allocatedBytes();
		// Direct setters accept an unchanged value without allocating a payload.
		if (mode <= 2) {
			EXPECT_TRUE(write(Voxel()));
			EXPECT_EQ(before, volume.allocatedBytes());
			EXPECT_FALSE(wrapper.dirtyRegion().isValid());
		}
		failChunk = true;
		const bool installed = SDL_SetMemoryFunctions(failingMalloc, originalCalloc, originalRealloc, originalFree);
		const bool written = write(value);
		const bool restored = SDL_SetMemoryFunctions(originalMalloc, originalCalloc, originalRealloc, originalFree);
		ASSERT_TRUE(installed);
		ASSERT_TRUE(restored);
		EXPECT_FALSE(failChunk);
		EXPECT_FALSE(written);
		EXPECT_EQ(Voxel(), volume.voxel(actual));
		EXPECT_EQ(before, volume.allocatedBytes());
		sampler.flush();
		EXPECT_FALSE(wrapper.dirtyRegion().isValid());
		EXPECT_TRUE(write(value));
		EXPECT_EQ(value, volume.voxel(actual));
		if (mode <= 2) {
			EXPECT_TRUE(write(value));
		}
		if (mode == 0) {
			EXPECT_EQ(Region(actual, actual), wrapper.dirtyRegion());
		}
	}
}

TEST_F(RawVolumeChunkTest, DenseAndChunkedQueriesAtCoordinateLimits) {
	const int previous = RawVolume::chunkedStorageThreshold();
	for (int threshold : {1, 256}) {
		RawVolume::setChunkedStorageThreshold(threshold);
		for (int origin : {INT_MIN, 0, INT_MAX - 2}) {
			RawVolume source(Region(origin, origin + 2));
			const Region selected(origin + 1, origin + 2);
			const Voxel value(VoxelType::Generic, 7);
			source.fill(value);
			RawVolume copy(source, selected);
			RawVolume target(selected);
			EXPECT_TRUE(target.copyInto(source, selected));
			EXPECT_EQ(value, target.voxel(origin + 2, origin + 2, origin + 2));
			EXPECT_EQ(value, copy.voxel(origin + 2, origin + 2, origin + 2));
			EXPECT_FALSE(source.isEmpty(selected));
			source.setFlags(selected, FlagOutline);
			EXPECT_TRUE(source.hasFlags(selected, FlagOutline));
			EXPECT_EQ(selected, source.regionForFlag(FlagOutline));
			source.toggleFlags(selected, FlagOutline);
			EXPECT_FALSE(source.hasFlags(selected, FlagOutline));
			source.setFlags(selected, FlagOutline);
			source.removeFlags(selected, FlagOutline);
			EXPECT_FALSE(source.hasFlags(selected, FlagOutline));
			source.clear();
			EXPECT_TRUE(source.isEmpty(selected));
		}
	}
	RawVolume::setChunkedStorageThreshold(previous);
}

TEST_F(RawVolumeChunkTest, SamplerNeighborsAtCoordinateLimits) {
	const int previous = RawVolume::chunkedStorageThreshold();
	for (int threshold : {1, 256}) {
		RawVolume::setChunkedStorageThreshold(threshold);
		for (int origin : {INT_MIN, 0, INT_MAX - 2}) {
			RawVolume volume(Region(origin, origin + 2));
			const Voxel inside(VoxelType::Generic, 7);
			const Voxel border(VoxelType::Generic, 9);
			volume.setBorderValue(border);
			// Leave chunks absent for one pass to exercise both fallback states.
			for (bool populated : {false, true}) {
				if (populated) {
					volume.setVoxel(origin, origin, origin, inside);
					volume.fill(inside);
				}
				const auto checkSampler = [&](auto &sampler) {
					for (int cell = 0; cell < 3; ++cell) {
						const int coord = origin + cell;
						EXPECT_TRUE(sampler.setPosition(coord, coord, coord));
						const Voxel neighbors[] = {
						sampler.peekVoxel1nx1ny1nz(),
						sampler.peekVoxel1nx1ny0pz(),
						sampler.peekVoxel1nx1ny1pz(),
						sampler.peekVoxel1nx0py1nz(),
						sampler.peekVoxel1nx0py0pz(),
						sampler.peekVoxel1nx0py1pz(),
						sampler.peekVoxel1nx1py1nz(),
						sampler.peekVoxel1nx1py0pz(),
						sampler.peekVoxel1nx1py1pz(),
						sampler.peekVoxel0px1ny1nz(),
						sampler.peekVoxel0px1ny0pz(),
						sampler.peekVoxel0px1ny1pz(),
						sampler.peekVoxel0px0py1nz(),
						sampler.peekVoxel0px0py0pz(),
						sampler.peekVoxel0px0py1pz(),
						sampler.peekVoxel0px1py1nz(),
						sampler.peekVoxel0px1py0pz(),
						sampler.peekVoxel0px1py1pz(),
						sampler.peekVoxel1px1ny1nz(),
						sampler.peekVoxel1px1ny0pz(),
						sampler.peekVoxel1px1ny1pz(),
						sampler.peekVoxel1px0py1nz(),
						sampler.peekVoxel1px0py0pz(),
						sampler.peekVoxel1px0py1pz(),
						sampler.peekVoxel1px1py1nz(),
						sampler.peekVoxel1px1py0pz(),
						sampler.peekVoxel1px1py1pz()};
						int index = 0;
						for (int x = -1; x <= 1; ++x) {
							for (int y = -1; y <= 1; ++y) {
								for (int z = -1; z <= 1; ++z) {
									const bool outside = cell + x < 0 || cell + x > 2 || cell + y < 0 ||
										cell + y > 2 || cell + z < 0 || cell + z > 2;
									EXPECT_EQ(outside ? border : populated ? inside : Voxel(), neighbors[index++]);
								}
							}
						}
					}
				};
				RawVolume::Sampler sampler(volume);
				checkSampler(sampler);
				RawVolumeWrapper wrapper(&volume);
				RawVolumeWrapper::Sampler wrapped(wrapper);
				checkSampler(wrapped);
				RawVolumeMoveWrapper moveWrapper(&volume);
				RawVolumeMoveWrapper::Sampler moved(moveWrapper);
				checkSampler(moved);
			}
		}
	}
	RawVolume::setChunkedStorageThreshold(previous);
}

TEST_F(RawVolumeChunkTest, QueriesAtMinimumCoordinate) {
	RawVolume volume(Region(INT_MIN, INT_MIN + 256));
	const Region cell(INT_MIN, INT_MIN);
	EXPECT_TRUE(volume.isEmpty(cell));
	EXPECT_FALSE(volume.hasFlags(cell, FlagOutline));
	volume.setFlags(cell, FlagOutline);
	EXPECT_TRUE(volume.hasFlags(cell, FlagOutline));
	volume.removeFlags(cell, FlagOutline);
	EXPECT_FALSE(volume.hasFlags(cell, FlagOutline));
	volume.toggleFlags(cell, FlagOutline);
	EXPECT_TRUE(volume.hasFlags(cell, FlagOutline));
}

TEST_F(RawVolumeChunkTest, CopyAtMaximumCoordinate) {
	RawVolume source(Region(INT_MAX - 256, INT_MAX));
	const Voxel value(VoxelType::Generic, 17, 11);
	source.setVoxel(INT_MAX, INT_MAX, INT_MAX, value);
	RawVolume copy(source, Region(INT_MAX - 1, INT_MAX));
	ASSERT_TRUE(copy.valid());
	EXPECT_EQ(value, copy.voxel(INT_MAX, INT_MAX, INT_MAX));
	EXPECT_EQ(Voxel(), copy.voxel(INT_MAX - 1, INT_MAX - 1, INT_MAX - 1));
	EXPECT_FALSE(source.isContiguous());
}

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

TEST_F(RawVolumeChunkTest, ConcurrentSamplersShareImmutableVolume) {
	RawVolume volume(largeRegion);
	const glm::ivec3 lower = largeRegion.getLowerCorner();
	for (int z = 0; z < 3; ++z) {
		for (int y = 0; y < 65; ++y) {
			for (int x = 0; x < 65; ++x) {
				volume.setVoxel(lower + glm::ivec3(x, y, z),
					Voxel(VoxelType::Generic, (uint8_t)(1 + (x + 3 * y + 7 * z) % 251)));
			}
		}
	}
	const size_t bytes = volume.allocatedBytes();
	core::AtomicInt mismatches;
	core::Thread readers[4];
	for (auto &reader : readers) {
		reader = core::Thread([&]() {
			RawVolume::Sampler sampler(volume);
			for (int z = 0; z < 4; ++z) {
				for (int y = 0; y < 66; ++y) {
					sampler.setPosition(lower + glm::ivec3(0, y, z));
					for (int x = 0; x < 257; ++x) {
						const uint8_t expected = x < 65 && y < 65 && z < 3
							? (uint8_t)(1 + (x + 3 * y + 7 * z) % 251) : 0;
						if (sampler.voxel().getColor() != expected) {
							++mismatches;
						}
						sampler.movePositiveX();
					}
				}
			}
		}, "rawvolume-reader");
		ASSERT_TRUE(reader.joinable());
	}
	for (auto &reader : readers) {
		reader.join();
	}
	EXPECT_EQ(0, (int)mismatches);
	EXPECT_EQ(bytes, volume.allocatedBytes());
	EXPECT_EQ(nullptr, volume.linearVoxels());
}

TEST_F(RawVolumeChunkTest, CyclicMoveStaysChunkedAndMatchesDense) {
	RawVolume volume(largeRegion);
	const size_t emptyBytes = volume.allocatedBytes();
	ASSERT_TRUE(volume.move({1, 32, -1}));
	EXPECT_EQ(emptyBytes, volume.allocatedBytes());
	RawVolume dense(largeRegion);
	ASSERT_NE(nullptr, dense.voxels());
	for (const glm::ivec3 shift : {glm::ivec3(1, 0, 0), glm::ivec3(32, -31, 257), glm::ivec3(-258, 258, -258)}) {
		volume.clear();
		dense.clear();
		const glm::ivec3 points[] = {largeRegion.getLowerCorner(), largeRegion.getLowerCorner() + 31,
			largeRegion.getLowerCorner() + 32, largeRegion.getUpperCorner()};
		uint8_t color = 1;
		for (const auto &point : points) {
			const Voxel value(VoxelType::Generic, color++);
			volume.setVoxel(point, value);
			dense.setVoxel(point, value);
		}
		ASSERT_TRUE(volume.move(shift));
		ASSERT_TRUE(dense.move(shift));
		Voxel *copy = volume.copyVoxels();
		ASSERT_NE(nullptr, copy);
		EXPECT_EQ(0, core_memcmp(copy, dense.linearVoxels(), RawVolume::size(largeRegion)));
		core_free(copy);
		EXPECT_EQ(nullptr, volume.linearVoxels());
		EXPECT_LT(volume.allocatedBytes(), RawVolume::size(largeRegion) / 10);
	}
}

TEST_F(RawVolumeChunkTest, ClearedCopyDoesNotRetainSourcePayload) {
	RawVolume source(largeRegion);
	const size_t directoryBytes = source.allocatedBytes();
	source.setVoxel(largeRegion.getLowerCorner(), Voxel(VoxelType::Generic, 17));
	source.setVoxel(largeRegion.getUpperCorner(), Voxel(VoxelType::Generic, 19));
	RawVolume::Sampler retained(source);
	ASSERT_TRUE(retained.setPosition(largeRegion.getLowerCorner()));
	source.clear();
	const size_t retainedBytes = source.allocatedBytes();
	ASSERT_GT(retainedBytes, directoryBytes);
	RawVolume copy(source);
	EXPECT_EQ(directoryBytes, copy.allocatedBytes());
	EXPECT_EQ(retainedBytes, source.allocatedBytes());
	EXPECT_EQ(Voxel(), retained.voxel());
	EXPECT_TRUE(copy.isEmpty(largeRegion));
	EXPECT_EQ(nullptr, copy.linearVoxels());
	copy.setVoxel(largeRegion.getLowerCorner(), Voxel(VoxelType::Generic, 23));
	EXPECT_EQ(Voxel(), source.voxel(largeRegion.getLowerCorner()));
}

TEST_F(RawVolumeChunkTest, CopyOfFullyAllocatedClearedVolumeUsesOnlyDirectory) {
	RawVolume source(Region(-128, 127));
	const size_t directoryBytes = source.allocatedBytes();
	for (int z = -128; z < 128; z += 32) {
		for (int y = -128; y < 128; y += 32) {
			for (int x = -128; x < 128; x += 32) {
				source.setVoxel(x, y, z, Voxel(VoxelType::Generic, 17));
			}
		}
	}
	ASSERT_EQ(directoryBytes + RawVolume::size(source.region()), source.allocatedBytes());
	source.clear();
	RawVolume copy(source);
	EXPECT_EQ(directoryBytes, copy.allocatedBytes());
	EXPECT_EQ(directoryBytes + RawVolume::size(source.region()), source.allocatedBytes());
	EXPECT_TRUE(copy.isEmpty(copy.region()));
	EXPECT_EQ(nullptr, copy.linearVoxels());
}

} // namespace voxel
