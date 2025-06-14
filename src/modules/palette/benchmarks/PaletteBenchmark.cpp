/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"

class PaletteBenchmark : public app::AbstractBenchmark {
protected:
	using Super = app::AbstractBenchmark;
	palette::Palette _palette;
	palette::PaletteLookup _paletteLookup;

public:
	void SetUp(::benchmark::State &state) override {
		Super::SetUp(state);
		_palette.nippon();
		_paletteLookup = palette::PaletteLookup(_palette);
	}
};

BENCHMARK_DEFINE_F(PaletteBenchmark, findReplacement)(benchmark::State &state) {
	for (auto _ : state) {
		benchmark::DoNotOptimize(_palette.findReplacement(13));
	}
}

BENCHMARK_DEFINE_F(PaletteBenchmark, paletteLookup)(benchmark::State &state) {
	int i = 0;
	for (auto _ : state) {
		benchmark::DoNotOptimize(_paletteLookup.findClosestIndex(core::RGBA((255 + i) % 255, (124 + 3 * i) % 255, (34 * i) % 255)));
		++i;
	}
}

BENCHMARK_REGISTER_F(PaletteBenchmark, findReplacement);
BENCHMARK_REGISTER_F(PaletteBenchmark, paletteLookup);
BENCHMARK_MAIN();
