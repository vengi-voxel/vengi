/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "core/ScopedPtr.h"
#include "palette/Palette.h"
#include "palette/PaletteView.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Voxel.h"
#include "voxelutil/FillHollow.h"
#include "voxelutil/Shadow.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeMerger.h"
#include "voxelutil/VolumeMover.h"
#include "voxelutil/VolumeRescaler.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelutil/VoxelUtil.h"

class VoxelUtilBenchmark : public app::AbstractBenchmark {
protected:
	voxel::RawVolume v{voxel::Region{-20, 20}};
};

BENCHMARK_DEFINE_F(VoxelUtilBenchmark, Crop)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::RawVolume out(voxel::Region{-20, 20});
		out.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		core::ScopedPtr<voxel::RawVolume> volume(voxelutil::cropVolume(&out));
		benchmark::DoNotOptimize(volume);
	}
}

BENCHMARK_DEFINE_F(VoxelUtilBenchmark, Move)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::RawVolume out(voxel::Region{-20, 20});
		voxel::RawVolume in(voxel::Region{-20, 20});
		in.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		voxelutil::moveVolume(&out, &in, glm::ivec3(1, 1, 1));
	}
}

BENCHMARK_DEFINE_F(VoxelUtilBenchmark, ScaleDown)(benchmark::State &state) {
	palette::Palette pal;
	pal.nippon();
	int color = 0;
	voxel::RawVolume in(voxel::Region{0, 20});
	in.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, color++ % palette::PaletteMaxColors));
	in.setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, color++ % palette::PaletteMaxColors));
	in.setVoxel(2, 2, 2, voxel::createVoxel(voxel::VoxelType::Generic, color++ % palette::PaletteMaxColors));
	in.setVoxel(2, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, color++ % palette::PaletteMaxColors));
	in.setVoxel(3, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, color++ % palette::PaletteMaxColors));
	in.setVoxel(4, 2, 1, voxel::createVoxel(voxel::VoxelType::Generic, color++ % palette::PaletteMaxColors));
	in.setVoxel(5, 3, 2, voxel::createVoxel(voxel::VoxelType::Generic, color++ % palette::PaletteMaxColors));
	in.setVoxel(5, 3, 3, voxel::createVoxel(voxel::VoxelType::Generic, color++ % palette::PaletteMaxColors));
	for (auto _ : state) {
		voxel::RawVolume out(voxel::Region{0, 10});
		voxelutil::scaleDown(in, pal, out);
	}
}

BENCHMARK_DEFINE_F(VoxelUtilBenchmark, ScaleUp)(benchmark::State &state) {
	int color = 0;
	voxel::RawVolume in(voxel::Region{0, 20});
	in.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, color++ % palette::PaletteMaxColors));
	in.setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, color++ % palette::PaletteMaxColors));
	in.setVoxel(2, 2, 2, voxel::createVoxel(voxel::VoxelType::Generic, color++ % palette::PaletteMaxColors));
	in.setVoxel(2, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, color++ % palette::PaletteMaxColors));
	in.setVoxel(3, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, color++ % palette::PaletteMaxColors));
	in.setVoxel(4, 2, 1, voxel::createVoxel(voxel::VoxelType::Generic, color++ % palette::PaletteMaxColors));
	in.setVoxel(5, 3, 2, voxel::createVoxel(voxel::VoxelType::Generic, color++ % palette::PaletteMaxColors));
	in.setVoxel(5, 3, 3, voxel::createVoxel(voxel::VoxelType::Generic, color++ % palette::PaletteMaxColors));
	for (auto _ : state) {
		voxel::RawVolume *scaled = voxelutil::scaleUp(in);
		if (scaled != nullptr) {
			benchmark::DoNotOptimize(scaled);
			delete scaled;
		}
	}
}

BENCHMARK_DEFINE_F(VoxelUtilBenchmark, FillHollow)(benchmark::State &state) {
	voxel::RawVolume in(voxel::Region{0, 20});
	voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 0);
	auto visitor = [&](int x, int y, int z, const voxel::Voxel &) { v.setVoxel(x, y, z, voxel); };
	voxelutil::visitVolumeParallel(in, visitor, voxelutil::VisitAll());
	in.setVoxel(in.region().getCenter(), voxel::Voxel());
	for (auto _ : state) {
		voxel::RawVolume copy(in);
		voxelutil::fillHollow(copy, voxel);
	}
}

BENCHMARK_DEFINE_F(VoxelUtilBenchmark, Merge)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::RawVolume out(voxel::Region{-20, 20});
		voxel::RawVolume in(voxel::Region{-20, 20});
		in.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		voxelutil::mergeVolumes(&out, &in, out.region(), in.region());
	}
}

BENCHMARK_DEFINE_F(VoxelUtilBenchmark, MergeSameDim)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::RawVolume out(voxel::Region{-20, 20});
		voxel::RawVolume in(voxel::Region{-20, 20});
		in.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		voxelutil::mergeRawVolumesSameDimension(&out, &in);
	}
}

BENCHMARK_DEFINE_F(VoxelUtilBenchmark, CopyIntoRegion)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::RawVolume out(voxel::Region{-19, 20});
		out.copyInto(v);
	}
}

BENCHMARK_DEFINE_F(VoxelUtilBenchmark, CopyViaRawVolume)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::RawVolume out(v, voxel::Region{-19, 20});
		benchmark::DoNotOptimize(out);
	}
}

BENCHMARK_DEFINE_F(VoxelUtilBenchmark, CopyIntoRegionSameDim)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::RawVolume out(voxel::Region{-20, 20});
		out.copyInto(v);
	}
}

BENCHMARK_DEFINE_F(VoxelUtilBenchmark, CopyViaRawVolumeMultipleRegions)(benchmark::State &state) {
	const core::Buffer<voxel::Region> regions {
		voxel::Region{-10, 10},
		voxel::Region{10, 20},
		voxel::Region{1, 1},
		voxel::Region{5, 5},
		voxel::Region{-20, -18},
		voxel::Region{-17, 10}
	};
	for (auto _ : state) {
		voxel::RawVolume out(v, regions);
		benchmark::DoNotOptimize(out);
	}
}

BENCHMARK_DEFINE_F(VoxelUtilBenchmark, CopyViaRawVolumeSameDim)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::RawVolume out(v, voxel::Region{-20, 20});
		benchmark::DoNotOptimize(out);
	}
}

BENCHMARK_DEFINE_F(VoxelUtilBenchmark, Shadow)(benchmark::State &state) {
	voxel::RawVolume in(voxel::Region{0, 20});
	voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 0);
	voxel::RawVolumeWrapper wrapper(&in);
	int n = voxelutil::extrudePlane(wrapper, in.region().getLowerCenter(), voxel::FaceNames::PositiveY, voxel::Voxel(),
									voxel, 1);
	core_assert_always(n == in.region().getWidthInVoxels() * in.region().getDepthInVoxels());
	in.setVoxel(in.region().getCenter(), voxel);
	palette::Palette palette;
	palette.nippon();
	for (auto _ : state) {
		voxelutil::shadow(in, palette);
	}
}

BENCHMARK_REGISTER_F(VoxelUtilBenchmark, ScaleDown);
BENCHMARK_REGISTER_F(VoxelUtilBenchmark, ScaleUp);
BENCHMARK_REGISTER_F(VoxelUtilBenchmark, Crop);
BENCHMARK_REGISTER_F(VoxelUtilBenchmark, FillHollow);
BENCHMARK_REGISTER_F(VoxelUtilBenchmark, Move);
BENCHMARK_REGISTER_F(VoxelUtilBenchmark, Merge);
BENCHMARK_REGISTER_F(VoxelUtilBenchmark, MergeSameDim);
BENCHMARK_REGISTER_F(VoxelUtilBenchmark, Shadow);
BENCHMARK_REGISTER_F(VoxelUtilBenchmark, CopyIntoRegion);
BENCHMARK_REGISTER_F(VoxelUtilBenchmark, CopyViaRawVolume);
BENCHMARK_REGISTER_F(VoxelUtilBenchmark, CopyIntoRegionSameDim);
BENCHMARK_REGISTER_F(VoxelUtilBenchmark, CopyViaRawVolumeSameDim);
BENCHMARK_REGISTER_F(VoxelUtilBenchmark, CopyViaRawVolumeMultipleRegions);
