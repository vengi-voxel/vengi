/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "color/ColorUtil.h"
#include "color/RGBA.h"
#include "palette/NormalPalette.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "palette/PaletteUtil.h"

class PaletteBenchmark : public app::AbstractBenchmark {
protected:
	using Super = app::AbstractBenchmark;
};

BENCHMARK_DEFINE_F(PaletteBenchmark, findReplacement)(benchmark::State &state) {
	palette::Palette palette;
	palette.nippon();
	for (auto _ : state) {
		benchmark::DoNotOptimize(palette.findReplacement(13));
	}
}

BENCHMARK_DEFINE_F(PaletteBenchmark, getClosestMatchApprox)(benchmark::State &state) {
	palette::Palette palette;
	palette.nippon();
	const color::RGBA target(13, 39, 26, 255);
	for (auto _ : state) {
		benchmark::DoNotOptimize(palette.getClosestMatch(target, -1, color::Distance::Approximation));
	}
}

BENCHMARK_DEFINE_F(PaletteBenchmark, getClosestMatchHSB)(benchmark::State &state) {
	palette::Palette palette;
	palette.nippon();
	const color::RGBA target(13, 39, 26, 255);
	for (auto _ : state) {
		benchmark::DoNotOptimize(palette.getClosestMatch(target, -1, color::Distance::HSB));
	}
}

BENCHMARK_DEFINE_F(PaletteBenchmark, paletteLookupConstruct)(benchmark::State &state) {
	palette::Palette palette;
	palette.nippon();
	for (auto _ : state) {
		palette::PaletteLookup palLookup(palette);
		benchmark::DoNotOptimize(palLookup);
	}
}

BENCHMARK_DEFINE_F(PaletteBenchmark, paletteLookupFind)(benchmark::State &state) {
	palette::Palette palette;
	palette.nippon();
	palette::PaletteLookup palLookup(palette);
	int i = 0;
	for (auto _ : state) {
		benchmark::DoNotOptimize(
			palLookup.findClosestIndex(color::RGBA((255 + i) % 255, (124 + 3 * i) % 255, (34 * i) % 255)));
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

BENCHMARK_DEFINE_F(PaletteBenchmark, hash)(benchmark::State &state) {
	palette::Palette palette;
	palette.nippon();
	for (auto _ : state) {
		palette.markDirty();
		benchmark::DoNotOptimize(palette.hash());
	}
}

BENCHMARK_DEFINE_F(PaletteBenchmark, hashCached)(benchmark::State &state) {
	palette::Palette palette;
	palette.nippon();
	(void)palette.hash();
	for (auto _ : state) {
		benchmark::DoNotOptimize(palette.hash());
	}
}

BENCHMARK_DEFINE_F(PaletteBenchmark, hasAnyEmit)(benchmark::State &state) {
	palette::Palette palette;
	palette.nippon();
	for (auto _ : state) {
		benchmark::DoNotOptimize(palette.hasAnyEmit());
	}
}

BENCHMARK_DEFINE_F(PaletteBenchmark, toColorPalette)(benchmark::State &state) {
	palette::Palette palette;
	palette.nippon();
	for (auto _ : state) {
		benchmark::DoNotOptimize(palette::toColorPalette(palette));
	}
}

BENCHMARK_DEFINE_F(PaletteBenchmark, toPalette)(benchmark::State &state) {
	palette::Palette palette;
	palette.nippon();
	palette::ColorPalette colorPalette = palette::toColorPalette(palette);
	for (auto _ : state) {
		benchmark::DoNotOptimize(palette::toPalette(colorPalette));
	}
}

BENCHMARK_REGISTER_F(PaletteBenchmark, findReplacement);
BENCHMARK_REGISTER_F(PaletteBenchmark, getClosestMatchApprox);
BENCHMARK_REGISTER_F(PaletteBenchmark, getClosestMatchHSB);
BENCHMARK_REGISTER_F(PaletteBenchmark, paletteLookupConstruct);
BENCHMARK_REGISTER_F(PaletteBenchmark, paletteLookupFind);
BENCHMARK_REGISTER_F(PaletteBenchmark, toVec4fPalette);
BENCHMARK_REGISTER_F(PaletteBenchmark, toVec4fNormalPalette);
BENCHMARK_REGISTER_F(PaletteBenchmark, hash);
BENCHMARK_REGISTER_F(PaletteBenchmark, hashCached);
BENCHMARK_REGISTER_F(PaletteBenchmark, hasAnyEmit);
BENCHMARK_REGISTER_F(PaletteBenchmark, toColorPalette);
BENCHMARK_REGISTER_F(PaletteBenchmark, toPalette);
BENCHMARK_MAIN();
