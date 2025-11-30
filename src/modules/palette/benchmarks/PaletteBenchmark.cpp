/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"

class PaletteBenchmark : public app::AbstractBenchmark {
protected:
	using Super = app::AbstractBenchmark;
};

BENCHMARK_DEFINE_F(PaletteBenchmark, findReplacement)(benchmark::State &state) {
	palette::Palette palette;
	palette.nippon();
	for (auto _ : state) {
		palette::PaletteLookup palLookup(palette);
		benchmark::DoNotOptimize(palette.findReplacement(13));
	}
}

BENCHMARK_DEFINE_F(PaletteBenchmark, getClosestMatch)(benchmark::State &state) {
	palette::Palette palette;
	palette.nippon();
	for (auto _ : state) {
		palette::PaletteLookup palLookup(palette);
		benchmark::DoNotOptimize(palette.getClosestMatch(color::RGBA(13, 39, 26, 255)));
	}
}

BENCHMARK_DEFINE_F(PaletteBenchmark, paletteLookup)(benchmark::State &state) {
	int i = 0;
	palette::Palette palette;
	palette.nippon();
	for (auto _ : state) {
		palette::PaletteLookup palLookup(palette);
		benchmark::DoNotOptimize(palLookup.findClosestIndex(color::RGBA((255 + i) % 255, (124 + 3 * i) % 255, (34 * i) % 255)));
		++i;
	}
}

BENCHMARK_REGISTER_F(PaletteBenchmark, findReplacement);
BENCHMARK_REGISTER_F(PaletteBenchmark, paletteLookup);
BENCHMARK_REGISTER_F(PaletteBenchmark, getClosestMatch);
BENCHMARK_MAIN();
