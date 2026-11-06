/**
 * @file
 */

#include "voxel/private/ChunkedVolumeStorage.h"
#include "core/StandardLib.h"
#include "core/concurrent/Atomic.h"
#include "core/concurrent/Thread.h"
#include <SDL3/SDL_timer.h>
#include <gtest/gtest.h>

namespace voxel {

TEST(ChunkedVolumeStorageTest, LazyAllocationAndBoundaries) {
	ChunkedVolumeStorage storage({65, 34, 33});
	ASSERT_TRUE(storage.valid());
	const size_t directoryBytes = 3 * 2 * 2 * sizeof(void *);
	EXPECT_EQ(directoryBytes, storage.allocatedBytes());
	const Voxel empty;
	for (int z = 0; z < 33; ++z) {
		for (int y = 0; y < 34; ++y) {
			for (int x = 0; x < 65; ++x) {
				EXPECT_EQ(empty, storage.voxel({x, y, z}));
			}
		}
	}
	EXPECT_FALSE(storage.setVoxel({32, 32, 32}, empty));
	EXPECT_EQ(directoryBytes, storage.allocatedBytes());
	const Voxel value(VoxelType::Generic, 17, 23, FlagOutline, 4);
	EXPECT_TRUE(storage.setVoxel({31, 31, 31}, value));
	EXPECT_TRUE(storage.setVoxel({32, 32, 32}, value));
	EXPECT_TRUE(storage.setVoxel({64, 33, 32}, value));
	EXPECT_EQ(directoryBytes + 3 * 32 * 32 * 32 * sizeof(Voxel), storage.allocatedBytes());
	EXPECT_EQ(value, storage.voxel({31, 31, 31}));
	EXPECT_EQ(value, storage.voxel({32, 32, 32}));
	EXPECT_EQ(value, storage.voxel({64, 33, 32}));
	EXPECT_EQ(empty, storage.voxel({32, 31, 31}));
	EXPECT_FALSE(storage.setVoxel({32, 32, 32}, value));
}

TEST(ChunkedVolumeStorageTest, FillCopyAndNormalAssignment) {
	ChunkedVolumeStorage storage({65, 2, 1});
	const Voxel filled(VoxelType::Generic, 7, 19, FlagOutline, 3);
	storage.fill(filled);
	const size_t directoryBytes = 3 * sizeof(void *);
	EXPECT_EQ(directoryBytes, storage.allocatedBytes());
	EXPECT_EQ(filled, storage.voxel({64, 1, 0}));
	// RawVolume assignment preserves the old normal when the new normal is unspecified.
	const Voxel replacement(VoxelType::Generic, 8);
	EXPECT_TRUE(storage.setVoxel({32, 1, 0}, replacement));
	EXPECT_EQ(19, storage.voxel({32, 1, 0}).getNormal());
	ChunkedVolumeStorage copy(storage);
	ASSERT_TRUE(copy.valid());
	EXPECT_EQ(storage.allocatedBytes(), copy.allocatedBytes());
	EXPECT_EQ(storage.voxel({32, 1, 0}), copy.voxel({32, 1, 0}));
	EXPECT_TRUE(copy.setVoxel({64, 1, 0}, Voxel(VoxelType::Transparent, 9)));
	EXPECT_EQ(filled, storage.voxel({64, 1, 0}));
	Voxel *linear = (Voxel *)core_malloc(65 * 2 * sizeof(Voxel));
	ASSERT_NE(nullptr, linear);
	copy.copyTo(linear);
	for (int y = 0; y < 2; ++y) {
		for (int x = 0; x < 65; ++x) {
			EXPECT_EQ(copy.voxel({x, y, 0}), linear[x + 65 * y]);
		}
	}
	core_free(linear);
	const Voxel *stable = &storage.voxel({32, 1, 0});
	storage.fill(Voxel());
	EXPECT_EQ(stable, &storage.voxel({32, 1, 0}));
	EXPECT_EQ(Voxel(), *stable);
	EXPECT_EQ(Voxel(), storage.voxel({64, 1, 0}));
}

TEST(ChunkedVolumeStorageTest, ConcurrentFirstWritesAndReadOnlyAccess) {
	ChunkedVolumeStorage storage({96, 32, 32});
	core::AtomicInt ready;
	core::AtomicBool start(false);
	core::Thread threads[4];
	for (int i = 0; i < 4; ++i) {
		threads[i] = core::Thread([&, i]() {
			++ready;
			while (!start) {
				SDL_Delay(1);
			}
			for (int x = 0; x < 64; ++x) {
				storage.setVoxel({x, i, 0}, Voxel(VoxelType::Generic, (uint8_t)(i + 1)));
			}
		}, "chunk-writer");
		if (!threads[i].joinable()) {
			start = true;
			FAIL() << "Could not start chunk writer";
		}
	}
	while ((int)ready != 4) {
		SDL_Delay(1);
	}
	start = true;
	for (auto &thread : threads) {
		thread.join();
	}
	const size_t bytes = storage.allocatedBytes();
	core::AtomicInt mismatches;
	for (auto &thread : threads) {
		thread = core::Thread([&]() {
			for (int repeat = 0; repeat < 100; ++repeat) {
				for (int y = 0; y < 32; ++y) {
					for (int x = 0; x < 96; ++x) {
						const uint8_t expected = y < 4 && x < 64 ? (uint8_t)(y + 1) : 0;
						if (storage.voxel({x, y, 0}).getColor() != expected) {
							++mismatches;
						}
					}
				}
			}
		}, "chunk-reader");
		ASSERT_TRUE(thread.joinable());
	}
	for (auto &thread : threads) {
		thread.join();
	}
	EXPECT_EQ(0, (int)mismatches);
	EXPECT_EQ(bytes, storage.allocatedBytes());
}

} // namespace voxel
