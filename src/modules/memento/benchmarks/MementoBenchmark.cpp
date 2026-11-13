/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "memento/MementoHandler.h"
#include "voxel/RawVolume.h"

class MementoBenchmark : public app::AbstractBenchmark {
protected:
	using Super = app::AbstractBenchmark;
};

BENCHMARK_DEFINE_F(MementoBenchmark, mementoDataCompress)(benchmark::State &state) {
	voxel::Region region(0, 0, 0, 127, 127, 127);
	voxel::RawVolume v(region);
	for (auto _ : state) {
		memento::MementoData mementoData = memento::MementoData::fromVolume(&v, region);
		benchmark::DoNotOptimize(mementoData);
	}
}

BENCHMARK_DEFINE_F(MementoBenchmark, mementoDataExtract)(benchmark::State &state) {
	voxel::Region region(0, 0, 0, 127, 127, 127);
	voxel::RawVolume v(region);
	memento::MementoData mementoData = memento::MementoData::fromVolume(&v, region);
	for (auto _ : state) {
		voxel::RawVolume volume(region);
		memento::MementoData::toVolume(&volume, mementoData, region);
		benchmark::DoNotOptimize(volume);
	}
}

BENCHMARK_REGISTER_F(MementoBenchmark, mementoDataCompress);
BENCHMARK_REGISTER_F(MementoBenchmark, mementoDataExtract);

BENCHMARK_DEFINE_F(MementoBenchmark, mementoSnapshot)(benchmark::State &state) {
	voxel::RawVolume volume(voxel::Region(-128, 127));
	if (state.range(0) && !volume.voxels()) {
		state.SkipWithError("Could not allocate dense source");
		return;
	}
	if (state.range(2)) {
		for (int z = -128; z <= 127; ++z) {
			for (int y = -128; y <= 127; ++y) {
				for (int x = -128; x <= 127; ++x) {
					volume.setVoxel(x, y, z, voxel::Voxel(voxel::VoxelType::Generic, (uint8_t)(x + 3 * y + 7 * z)));
				}
			}
		}
	}
	const voxel::Region region = state.range(1) ? voxel::Region(-32, 31) : volume.region();
	const size_t sourceBytes = volume.allocatedBytes();
	for (auto _ : state) {
		memento::MementoData data = memento::MementoData::fromVolume(&volume, region);
		benchmark::DoNotOptimize(data);
	}
	state.counters["source_allocated_bytes"] = (double)volume.allocatedBytes();
	if (sourceBytes != volume.allocatedBytes()) {
		state.SkipWithError("Snapshot changed source allocation");
	}
}

// dense source, subregion, nonuniform content
BENCHMARK_REGISTER_F(MementoBenchmark, mementoSnapshot)->ArgsProduct({{0, 1}, {0, 1}, {0, 1}});
BENCHMARK_MAIN();
