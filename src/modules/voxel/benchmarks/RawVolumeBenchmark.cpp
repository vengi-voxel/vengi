/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Vector.h"
#include "math/Axis.h"
#include "voxel/RawVolume.h"
#include "voxel/VolumeSamplerUtil.h"

class RawVolumeBenchmark : public app::AbstractBenchmark {
protected:
	voxel::RawVolume v{voxel::Region{0, 0, 0, 143, 22, 134}};
};

BENCHMARK_DEFINE_F(RawVolumeBenchmark, SetVoxel)(benchmark::State &state) {
	for (auto _ : state) {
		v.setVoxel(96, 6, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 7, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
}

BENCHMARK_DEFINE_F(RawVolumeBenchmark, IsEmpty)(benchmark::State &state) {
	for (auto _ : state) {
		benchmark::DoNotOptimize(v.isEmpty(v.region()));
	}
}

BENCHMARK_DEFINE_F(RawVolumeBenchmark, SetVoxelSampler)(benchmark::State &state) {
	voxel::RawVolume::Sampler sampler(&v);
	for (auto _ : state) {
		sampler.setPosition(96, 6, 62);
		sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
		sampler.movePositiveY();
		sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
}

BENCHMARK_DEFINE_F(RawVolumeBenchmark, SetVoxelsY)(benchmark::State &state) {
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	core::Vector<voxel::Voxel, 22> voxels;
	voxels.assign(voxel, voxels.capacity());
	for (auto _ : state) {
		voxel::setVoxels(v, 0, 0, &voxels.front(), v.region().getHeightInVoxels());
	}
}

BENCHMARK_DEFINE_F(RawVolumeBenchmark, SetVoxels)(benchmark::State &state) {
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	core::Vector<voxel::Voxel, 22> voxels;
	voxels.assign(voxel, voxels.capacity());

	for (auto _ : state) {
		voxel::setVoxels(v, 0, 0, 0, v.region().getWidthInVoxels(), v.region().getDepthInVoxels(), &voxels.front(),
				  v.region().getHeightInVoxels());
	}
}

BENCHMARK_DEFINE_F(RawVolumeBenchmark, RegionForFlag)(benchmark::State &state) {
	voxel::RawVolume in(voxel::Region{-20, 20});
	voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	voxel.setOutline();
	in.setVoxel(-10, -10, -10, voxel);
	in.setVoxel(10, 10, 10, voxel);
	for (auto _ : state) {
		voxel::Region r = in.regionForFlag(voxel::FlagOutline);
		benchmark::DoNotOptimize(r);
	}
}

BENCHMARK_DEFINE_F(RawVolumeBenchmark, HasFlagsNoFlags)(benchmark::State &state) {
	voxel::RawVolume in(voxel::Region{-100, 100});
	in.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	for (auto _ : state) {
		benchmark::DoNotOptimize(in.hasFlags(in.region(), voxel::FlagOutline));
	}
}

BENCHMARK_DEFINE_F(RawVolumeBenchmark, HasFlagsWithFlags)(benchmark::State &state) {
	voxel::RawVolume in(voxel::Region{-100, 100});
	voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	voxel.setOutline();
	in.setVoxel(-10, -10, -10, voxel);
	in.setVoxel(10, 10, 10, voxel);
	for (auto _ : state) {
		benchmark::DoNotOptimize(in.hasFlags(in.region(), voxel::FlagOutline));
	}
}

BENCHMARK_DEFINE_F(RawVolumeBenchmark, HasFlagsSubRegion)(benchmark::State &state) {
	voxel::RawVolume in(voxel::Region{-100, 100});
	voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	voxel.setOutline();
	in.setVoxel(5, 5, 5, voxel);
	const voxel::Region subRegion(0, 0, 0, 10, 10, 10);
	for (auto _ : state) {
		benchmark::DoNotOptimize(in.hasFlags(subRegion, voxel::FlagOutline));
	}
}

static void fillBenchmarkVolume(voxel::RawVolume &volume, int content) {
	if (content == 0) {
		return;
	}
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	if (content == 2) {
		volume.fill(voxel);
		return;
	}
	const voxel::Region &region = volume.region();
	for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
		for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
			for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
				if (content == 3) {
					volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, (uint8_t)(x + 3 * y + 7 * z)));
				} else if (x == region.getLowerX() || x == region.getUpperX() || y == region.getLowerY() ||
					y == region.getUpperY() || z == region.getLowerZ() || z == region.getUpperZ()) {
					volume.setVoxel(x, y, z, voxel);
				}
			}
		}
	}
}

BENCHMARK_DEFINE_F(RawVolumeBenchmark, FillBehindPlane)(benchmark::State &state) {
	const int side = (int)state.range(0);
	const math::Axis axis = state.range(1) == 0 ? math::Axis::X : state.range(1) == 1 ? math::Axis::Y : math::Axis::Z;
	voxel::RawVolume volume(voxel::Region(0, side - 1));
	fillBenchmarkVolume(volume, (int)state.range(2));
	const int dAxis = math::getIndexForAxis(axis);
	const int aAxis = math::getIndexForAxis(math::getPlaneAxisA(axis));
	const int bAxis = math::getIndexForAxis(math::getPlaneAxisB(axis));
	const int slice = side / 2;
	const int cellCount = side * side;
	core::DynamicArray<voxel::Voxel> cells;
	cells.resize((size_t)cellCount);
	for (auto _ : state) {
		for (int b = 0; b < side; ++b) {
			for (int a = 0; a < side; ++a) {
				glm::ivec3 pos(0);
				pos[dAxis] = slice;
				pos[aAxis] = a;
				pos[bAxis] = b;
				cells[b * side + a] = volume.voxel(pos);
			}
		}
		volume.fillBehindPlane(axis, slice, 0, side - 1, 0, side - 1, &cells[0]);
		benchmark::DoNotOptimize(cells[0]);
	}
}

// Compare facade lookups with sampler traversal before changing the storage.
// Include nonzero origins and empty, shell and dense volumes.
BENCHMARK_DEFINE_F(RawVolumeBenchmark, ReadVoxels)(benchmark::State &state) {
	const int side = (int)state.range(0);
	const int lower = -side / 2;
	voxel::RawVolume volume(voxel::Region(lower, lower + side - 1));
	if (state.range(2)) {
		if (volume.voxels() == nullptr) {
			state.SkipWithError("Could not allocate dense comparison volume");
			return;
		}
	}
	fillBenchmarkVolume(volume, (int)state.range(1));
	const voxel::Region &region = volume.region();
	for (auto _ : state) {
		for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
			for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
				for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
					benchmark::DoNotOptimize(volume.voxel(x, y, z).getColor());
				}
			}
		}
	}
	state.SetItemsProcessed(state.iterations() * region.voxels());
	state.counters["allocated_bytes"] = (double)volume.allocatedBytes();
}

BENCHMARK_DEFINE_F(RawVolumeBenchmark, ReadVoxelsSampler)(benchmark::State &state) {
	const int side = (int)state.range(0);
	const int lower = -side / 2;
	voxel::RawVolume volume(voxel::Region(lower, lower + side - 1));
	if (state.range(2)) {
		if (volume.voxels() == nullptr) {
			state.SkipWithError("Could not allocate dense comparison volume");
			return;
		}
	}
	fillBenchmarkVolume(volume, (int)state.range(1));
	const voxel::Region &region = volume.region();
	voxel::RawVolume::Sampler sampler(volume);
	for (auto _ : state) {
		for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
			for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
				sampler.setPosition(region.getLowerX(), y, z);
				for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
					benchmark::DoNotOptimize(sampler.voxel().getColor());
					sampler.movePositiveX();
				}
			}
		}
	}
	state.SetItemsProcessed(state.iterations() * region.voxels());
	state.counters["allocated_bytes"] = (double)volume.allocatedBytes();
}

BENCHMARK_REGISTER_F(RawVolumeBenchmark, ReadVoxels)->ArgsProduct({{32, 64, 128, 256}, {0, 1, 2, 3}, {0, 1}});
BENCHMARK_REGISTER_F(RawVolumeBenchmark, ReadVoxelsSampler)->ArgsProduct({{32, 64, 128, 256}, {0, 1, 2, 3}, {0, 1}});

static void cyclicVolumeMove(benchmark::State &state) {
	voxel::RawVolume volume(voxel::Region(-128, 127));
	if (state.range(0) && volume.voxels() == nullptr) {
		state.SkipWithError("Could not allocate dense comparison volume");
		return;
	}
	const voxel::Voxel value(voxel::VoxelType::Generic, 17);
	if (state.range(1) == 1) {
		volume.setVoxel(-128, -128, -128, value);
		volume.setVoxel(-97, -97, -97, value);
		volume.setVoxel(127, 127, 127, value);
	} else if (state.range(1) == 2) {
		volume.fill(value);
	} else if (state.range(1) == 3) {
		fillBenchmarkVolume(volume, 3);
	}
	for (auto _ : state) {
		if (!volume.move({1, 0, 0}) || !volume.move({-1, 0, 0})) {
			state.SkipWithError("Could not shift volume");
			break;
		}
		benchmark::DoNotOptimize(volume.linearVoxels());
	}
	state.counters["allocated_bytes"] = (double)volume.allocatedBytes();
}

BENCHMARK(cyclicVolumeMove)->ArgsProduct({{0, 1}, {0, 1, 2, 3}});

// Every benchmark thread reads the same immutable volume. Each owns its sampler.
// A plain benchmark avoids creating application fixtures concurrently.
template<int Content, bool Dense>
static void sharedVolumeRead(benchmark::State &state) {
	struct SharedVolume {
		voxel::RawVolume volume{voxel::Region(-128, 127)};
		bool ready = false;
		SharedVolume() {
			if constexpr (Dense) {
				if (volume.voxels() == nullptr) {
					return;
				}
			}
			fillBenchmarkVolume(volume, Content);
			ready = volume.valid();
		}
	};
	static const SharedVolume shared;
	const voxel::RawVolume &volume = shared.volume;
	if (!shared.ready) {
		state.SkipWithError("Could not allocate shared volume");
		return;
	}
	voxel::RawVolume::Sampler sampler(volume);
	const voxel::Region &region = volume.region();
	for (auto _ : state) {
		for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
			for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
				sampler.setPosition(region.getLowerX(), y, z);
				for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
					benchmark::DoNotOptimize(sampler.voxel().getColor());
					sampler.movePositiveX();
				}
			}
		}
	}
	state.SetItemsProcessed(state.iterations() * region.voxels());
}

BENCHMARK_TEMPLATE(sharedVolumeRead, 0, false)->ThreadRange(1, 4)->UseRealTime();
BENCHMARK_TEMPLATE(sharedVolumeRead, 0, true)->ThreadRange(1, 4)->UseRealTime();
BENCHMARK_TEMPLATE(sharedVolumeRead, 1, false)->ThreadRange(1, 4)->UseRealTime();
BENCHMARK_TEMPLATE(sharedVolumeRead, 1, true)->ThreadRange(1, 4)->UseRealTime();
BENCHMARK_TEMPLATE(sharedVolumeRead, 3, false)->ThreadRange(1, 4)->UseRealTime();
BENCHMARK_TEMPLATE(sharedVolumeRead, 3, true)->ThreadRange(1, 4)->UseRealTime();

BENCHMARK_REGISTER_F(RawVolumeBenchmark, SetVoxel);
BENCHMARK_REGISTER_F(RawVolumeBenchmark, SetVoxelSampler);
BENCHMARK_REGISTER_F(RawVolumeBenchmark, IsEmpty);
BENCHMARK_REGISTER_F(RawVolumeBenchmark, SetVoxelsY);
BENCHMARK_REGISTER_F(RawVolumeBenchmark, SetVoxels);
BENCHMARK_REGISTER_F(RawVolumeBenchmark, RegionForFlag);
BENCHMARK_REGISTER_F(RawVolumeBenchmark, HasFlagsSubRegion);
BENCHMARK_REGISTER_F(RawVolumeBenchmark, HasFlagsNoFlags);
BENCHMARK_REGISTER_F(RawVolumeBenchmark, HasFlagsWithFlags);
BENCHMARK_REGISTER_F(RawVolumeBenchmark, FillBehindPlane)
	->ArgsProduct({{64, 128}, {0, 1, 2}, {0, 1, 2}})
	->Unit(benchmark::kMicrosecond);
