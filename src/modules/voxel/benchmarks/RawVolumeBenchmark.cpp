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
				if (x == region.getLowerX() || x == region.getUpperX() || y == region.getLowerY() ||
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
