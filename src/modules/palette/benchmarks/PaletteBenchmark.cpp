/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "palette/NormalPalette.h"
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

BENCHMARK_DEFINE_F(PaletteBenchmark, toVec4fPalette)(benchmark::State &state) {
	palette::Palette palette;
	palette.nippon();
	for (auto _ : state) {
		glm::highp_vec4 materialColors[256];
		glm::highp_vec4 emitColors[256];
		palette.toVec4f(materialColors, emitColors);
		benchmark::DoNotOptimize(materialColors);
		benchmark::DoNotOptimize(emitColors);
	}
}

BENCHMARK_DEFINE_F(PaletteBenchmark, toVec4fNormalPalette)(benchmark::State &state) {
	palette::NormalPalette normalPalette;
	normalPalette.redAlert2();
	glm::highp_vec4 vec4f[palette::NormalPaletteMaxNormals];
	for (auto _ : state) {
		normalPalette.toVec4f(vec4f);
		benchmark::DoNotOptimize(vec4f);
	}
}

BENCHMARK_REGISTER_F(PaletteBenchmark, findReplacement);
BENCHMARK_REGISTER_F(PaletteBenchmark, paletteLookup);
BENCHMARK_REGISTER_F(PaletteBenchmark, getClosestMatch);
BENCHMARK_REGISTER_F(PaletteBenchmark, toVec4fPalette);
BENCHMARK_REGISTER_F(PaletteBenchmark, toVec4fNormalPalette);
BENCHMARK_MAIN();
