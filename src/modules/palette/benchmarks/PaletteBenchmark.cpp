/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "palette/Palette.h"

class PaletteBenchmark : public app::AbstractBenchmark {
protected:
	using Super = app::AbstractBenchmark;
	palette::Palette _palette;

public:
	void SetUp(::benchmark::State &state) override {
		Super::SetUp(state);
		_palette.nippon();
	}
};

BENCHMARK_DEFINE_F(PaletteBenchmark, findReplacement)(benchmark::State &state) {
	for (auto _ : state) {
		benchmark::DoNotOptimize(_palette.findReplacement(13));
	}
}

BENCHMARK_REGISTER_F(PaletteBenchmark, findReplacement);
BENCHMARK_MAIN();
