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
BENCHMARK_MAIN();
