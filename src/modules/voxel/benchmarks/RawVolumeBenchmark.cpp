/**
 * @file
 */

#include "app/ForParallel.h"
#include "app/benchmark/AbstractBenchmark.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Vector.h"
#include "core/concurrent/Atomic.h"
#include "core/concurrent/Concurrency.h"
#include "math/Axis.h"
#include "voxel/RawVolume.h"
#include "voxel/VolumeSamplerDispatch.h"
#include "voxel/VolumeSamplerUtil.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeMerger.h"
#include "voxelutil/VolumeMover.h"
#include "voxelutil/VolumeVisitor.h"

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

BENCHMARK_DEFINE_F(RawVolumeBenchmark, SetVoxelSamplerDispatched)(benchmark::State &state) {
	voxel::withVolumeSampler(v, [&](auto &sampler) {
		for (auto _ : state) {
			sampler.setPosition(96, 6, 62);
			sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
			sampler.movePositiveY();
			sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
		}
	});
}

BENCHMARK_DEFINE_F(RawVolumeBenchmark, SetVoxelsY)(benchmark::State &state) {
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	core::Vector<voxel::Voxel, 23> voxels;
	voxels.assign(voxel, voxels.capacity());
	for (auto _ : state) {
		voxel::setVoxels(v, 0, 0, &voxels.front(), v.region().getHeightInVoxels());
	}
}

BENCHMARK_DEFINE_F(RawVolumeBenchmark, SetVoxels)(benchmark::State &state) {
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	core::Vector<voxel::Voxel, 23> voxels;
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
	if (content == 4) {
		// Scattered voxels: 1 per 32^3 cell
		for (int z = region.getLowerZ(); z <= region.getUpperZ(); z += 32) {
			for (int y = region.getLowerY(); y <= region.getUpperY(); y += 32) {
				for (int x = region.getLowerX(); x <= region.getUpperX(); x += 32) {
					volume.setVoxel(x, y, z, voxel);
				}
			}
		}
		return;
	}
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

BENCHMARK_REGISTER_F(RawVolumeBenchmark, ReadVoxels)->ArgsProduct({{32, 64, 128, 256}, {0, 1, 2, 3, 4}, {0, 1}});
BENCHMARK_REGISTER_F(RawVolumeBenchmark, ReadVoxelsSampler)->ArgsProduct({{32, 64, 128, 256}, {0, 1, 2, 3, 4}, {0, 1}});

BENCHMARK_DEFINE_F(RawVolumeBenchmark, cyclicVolumeMove)(benchmark::State &state) {
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

BENCHMARK_REGISTER_F(RawVolumeBenchmark, cyclicVolumeMove)->ArgsProduct({{0, 1}, {0, 1, 2, 3}});

BENCHMARK_DEFINE_F(RawVolumeBenchmark, copyVolumeStorage)(benchmark::State &state) {
	voxel::RawVolume source(voxel::Region(-128, 127));
	if (!source.valid() || (state.range(0) && source.voxels() == nullptr)) {
		state.SkipWithError("Could not allocate source volume");
		return;
	}
	fillBenchmarkVolume(source, 3);
	if (state.range(1)) {
		source.clear();
	}
	size_t copyBytes = 0;
	for (auto _ : state) {
		voxel::RawVolume copy(source);
		if (!copy.valid()) {
			state.SkipWithError("Could not copy volume");
			break;
		}
		copyBytes = copy.allocatedBytes();
		benchmark::DoNotOptimize(copy.voxel(-128, -128, -128).getColor());
	}
	state.counters["source_allocated_bytes"] = (double)source.allocatedBytes();
	state.counters["copy_allocated_bytes"] = (double)copyBytes;
}

BENCHMARK_REGISTER_F(RawVolumeBenchmark, copyVolumeStorage)->ArgsProduct({{0, 1}, {0, 1}});

BENCHMARK_DEFINE_F(RawVolumeBenchmark, Large512MemoryAndBounds)(benchmark::State &state) {
	const int content = (int)state.range(0);
	voxel::RawVolume volume(voxel::Region(-256, 255));
	fillBenchmarkVolume(volume, content);
	for (auto _ : state) {
		glm::ivec3 mins, maxs;
		benchmark::DoNotOptimize(voxelutil::computeSolidBounds(&volume, mins, maxs));
		benchmark::DoNotOptimize(mins);
		benchmark::DoNotOptimize(maxs);
	}
	state.counters["allocated_bytes"] = (double)volume.allocatedBytes();
}

BENCHMARK_REGISTER_F(RawVolumeBenchmark, Large512MemoryAndBounds)->Arg(0)->Arg(1)->Arg(2)->Arg(4)->Unit(benchmark::kMicrosecond);

class ParallelRawVolumeBenchmark : public app::AbstractBenchmark {
public:
	ParallelRawVolumeBenchmark() : app::AbstractBenchmark(core::cpus()) {
	}
};

// Benchmark key voxelutil algorithms on 256^3 with full thread pool:
// content={1:shell, 3:dense, 4:scattered}, forceDense={0:sparse, 1:dense}
BENCHMARK_DEFINE_F(ParallelRawVolumeBenchmark, cyclicVolumeMoveMT)(benchmark::State &state) {
	voxel::RawVolume volume(voxel::Region(-128, 127));
	if (!volume.valid() || (state.range(0) && volume.voxels() == nullptr)) {
		state.SkipWithError("Could not allocate volume");
		return;
	}
	const int content = (int)state.range(1);
	if (content == 1) {
		volume.setVoxel(-128, -128, -128, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		volume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 2));
		volume.setVoxel(127, 127, 127, voxel::createVoxel(voxel::VoxelType::Generic, 3));
	} else if (content == 2) {
		fillBenchmarkVolume(volume, 2);
	} else if (content == 3) {
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

BENCHMARK_DEFINE_F(ParallelRawVolumeBenchmark, VoxelUtilMerge256)(benchmark::State &state) {
	const int content = (int)state.range(0);
	const bool forceDense = state.range(1) != 0;
	voxel::RawVolume src(voxel::Region(-128, 127));
	if (forceDense) {
		(void)src.voxels();
	}
	fillBenchmarkVolume(src, content);
	for (auto _ : state) {
		voxel::RawVolume dst(voxel::Region(-128, 127));
		if (forceDense) {
			(void)dst.voxels();
		}
		const int merged = voxelutil::mergeVolumes(&dst, &src, dst.region(), src.region());
		benchmark::DoNotOptimize(merged);
	}
	state.counters["src_bytes"] = (double)src.allocatedBytes();
}

BENCHMARK_DEFINE_F(ParallelRawVolumeBenchmark, VoxelUtilCountParallel256)(benchmark::State &state) {
	const int content = (int)state.range(0);
	const bool forceDense = state.range(1) != 0;
	voxel::RawVolume vol(voxel::Region(-128, 127));
	if (forceDense) {
		(void)vol.voxels();
	}
	fillBenchmarkVolume(vol, content);
	for (auto _ : state) {
		benchmark::DoNotOptimize(voxelutil::countVoxels(vol));
	}
	state.counters["bytes"] = (double)vol.allocatedBytes();
}

BENCHMARK_DEFINE_F(ParallelRawVolumeBenchmark, VoxelUtilVisitParallelSum256)(benchmark::State &state) {
	const int content = (int)state.range(0);
	const bool forceDense = state.range(1) != 0;
	voxel::RawVolume vol(voxel::Region(-128, 127));
	if (forceDense) {
		(void)vol.voxels();
	}
	fillBenchmarkVolume(vol, content);
	for (auto _ : state) {
		core::AtomicInt sum(0);
		voxelutil::visitVolumeParallel(vol, [&](int, int, int, const voxel::Voxel &v) {
			if ((v.getColor() & 63u) == 0u) {
				sum.increment(1);
			}
		});
		benchmark::DoNotOptimize((int)sum);
	}
	state.counters["bytes"] = (double)vol.allocatedBytes();
}

BENCHMARK_DEFINE_F(RawVolumeBenchmark, VoxelUtilVisitSolidZYX256)(benchmark::State &state) {
	const int content = (int)state.range(0);
	const bool forceDense = state.range(1) != 0;
	voxel::RawVolume vol(voxel::Region(-128, 127));
	if (forceDense) {
		(void)vol.voxels();
	}
	fillBenchmarkVolume(vol, content);
	for (auto _ : state) {
		int sum = 0;
		voxelutil::visitVolume(vol, [&](int, int, int, const voxel::Voxel &v) { sum += v.getColor(); });
		benchmark::DoNotOptimize(sum);
	}
	state.counters["bytes"] = (double)vol.allocatedBytes();
}

BENCHMARK_DEFINE_F(ParallelRawVolumeBenchmark, VoxelUtilInPlaceRecolor256)(benchmark::State &state) {
	const int content = (int)state.range(0);
	const bool forceDense = state.range(1) != 0;
	voxel::RawVolume vol(voxel::Region(-128, 127));
	if (forceDense) {
		(void)vol.voxels();
	}
	fillBenchmarkVolume(vol, content);
	const voxel::Region &reg = vol.region();
	for (auto _ : state) {
		app::for_parallel(reg.getLowerZ(), reg.getUpperZ() + 1, [&](int start, int end) {
			voxel::RawVolume::Sampler sampler(&vol);
			sampler.setPosition(reg.getLowerX(), reg.getLowerY(), start);
			for (int z = start; z < end; ++z) {
				voxel::RawVolume::Sampler sampler2 = sampler;
				for (int y = reg.getLowerY(); y <= reg.getUpperY(); ++y) {
					voxel::RawVolume::Sampler sampler3 = sampler2;
					for (int x = reg.getLowerX(); x <= reg.getUpperX(); ++x) {
						voxel::Voxel v = sampler3.voxel();
						if (!voxel::isAir(v.getMaterial())) {
							v.setColor((uint8_t)(v.getColor() ^ 1u));
							sampler3.setVoxel(v);
						}
						sampler3.movePositiveX();
					}
					sampler2.movePositiveY();
				}
				sampler.movePositiveZ();
			}
		});
		benchmark::DoNotOptimize(vol.allocatedBytes());
	}
	state.counters["bytes"] = (double)vol.allocatedBytes();
}

BENCHMARK_REGISTER_F(ParallelRawVolumeBenchmark, cyclicVolumeMoveMT)->ArgsProduct({{0, 1}, {1, 3}})->UseRealTime()->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(ParallelRawVolumeBenchmark, VoxelUtilMerge256)->ArgsProduct({{1, 3, 4}, {0, 1}})->UseRealTime()->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(ParallelRawVolumeBenchmark, VoxelUtilCountParallel256)->ArgsProduct({{1, 3, 4}, {0, 1}})->UseRealTime()->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(ParallelRawVolumeBenchmark, VoxelUtilVisitParallelSum256)->ArgsProduct({{1, 3, 4}, {0, 1}})->UseRealTime()->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(RawVolumeBenchmark, VoxelUtilVisitSolidZYX256)->ArgsProduct({{1, 3, 4}, {0, 1}})->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(ParallelRawVolumeBenchmark, VoxelUtilInPlaceRecolor256)->ArgsProduct({{1, 3, 4}, {0, 1}})->UseRealTime()->Unit(benchmark::kMillisecond);

class SharedVolumeBenchmark : public app::AbstractBenchmark {
protected:
	voxel::RawVolume *_volume = nullptr;
	using Super = app::AbstractBenchmark;

	void SetUp(benchmark::State &state) override {
		if (state.thread_index() == 0) {
			Super::SetUp(state);
			_volume = new voxel::RawVolume(voxel::Region(-128, 127));
			if (_volume->valid() && (!state.range(1) || _volume->voxels() != nullptr)) {
				fillBenchmarkVolume(*_volume, (int)state.range(0));
			}
		}
	}

	void TearDown(benchmark::State &state) override {
		if (state.thread_index() == 0) {
			delete _volume;
			_volume = nullptr;
			Super::TearDown(state);
		}
	}
};

BENCHMARK_DEFINE_F(SharedVolumeBenchmark, sharedVolumeRead)(benchmark::State &state) {
	while (state.KeepRunning()) {
		const voxel::RawVolume &volume = *_volume;
		if (!volume.valid() || (state.range(1) && !volume.isContiguous())) {
			state.SkipWithError("Could not allocate shared volume");
			continue;
		}
		voxel::RawVolume::Sampler sampler(volume);
		const voxel::Region &region = volume.region();
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
	state.SetItemsProcessed(state.iterations() * 256 * 256 * 256);
}

BENCHMARK_REGISTER_F(SharedVolumeBenchmark, sharedVolumeRead)
	->ArgsProduct({{0, 1, 3}, {0, 1}})->ThreadRange(1, 4)->UseRealTime();

BENCHMARK_REGISTER_F(RawVolumeBenchmark, SetVoxel);
BENCHMARK_REGISTER_F(RawVolumeBenchmark, SetVoxelSampler);
BENCHMARK_REGISTER_F(RawVolumeBenchmark, SetVoxelSamplerDispatched);
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
