#include "app/benchmark/AbstractBenchmark.h"
#include "benchmark/benchmark.h"
#include "color/ColorUtil.h"
#include "color/RGBA.h"

class ColorUtilBenchmark : public app::AbstractBenchmark {};

BENCHMARK_DEFINE_F(ColorUtilBenchmark, fromRGBA)(benchmark::State &state) {
	color::RGBA rgba(255, 0, 0, 255);
	for (auto _ : state) {
		benchmark::DoNotOptimize(color::fromRGBA(rgba));
	}
}

BENCHMARK_DEFINE_F(ColorUtilBenchmark, fromRGBA2)(benchmark::State &state) {
	color::RGBA rgba(255, 0, 0, 255);
	for (auto _ : state) {
		benchmark::DoNotOptimize(color::fromRGBA(rgba.r, rgba.g, rgba.b, rgba.a));
	}
}

BENCHMARK_DEFINE_F(ColorUtilBenchmark, getRGBA4)(benchmark::State &state) {
	for (auto _ : state) {
		benchmark::DoNotOptimize(color::toRGBA({0.0f, 0.0f, 0.0f, 0.0f}));
	}
}

BENCHMARK_DEFINE_F(ColorUtilBenchmark, getRGBA3)(benchmark::State &state) {
	for (auto _ : state) {
		benchmark::DoNotOptimize(color::getRGBA({0.0f, 0.0f, 0.0f}));
	}
}

BENCHMARK_DEFINE_F(ColorUtilBenchmark, getHSB)(benchmark::State &state) {
	for (auto _ : state) {
		float hue, saturation, brightness;
		glm::vec4 c(0.5f, 0.6f, 0.7f, 1.0f);
		color::getHSB(c, hue, saturation, brightness);
		benchmark::DoNotOptimize(hue);
		benchmark::DoNotOptimize(saturation);
		benchmark::DoNotOptimize(brightness);
	}
}

BENCHMARK_DEFINE_F(ColorUtilBenchmark, fromHSB)(benchmark::State &state) {
	for (auto _ : state) {
		benchmark::DoNotOptimize(color::fromHSB(0.5f, 0.5f, 0.5f, 1.0f));
	}
}

BENCHMARK_REGISTER_F(ColorUtilBenchmark, fromRGBA);
BENCHMARK_REGISTER_F(ColorUtilBenchmark, fromRGBA2);
BENCHMARK_REGISTER_F(ColorUtilBenchmark, getRGBA4);
BENCHMARK_REGISTER_F(ColorUtilBenchmark, getRGBA3);
BENCHMARK_REGISTER_F(ColorUtilBenchmark, getHSB);
BENCHMARK_REGISTER_F(ColorUtilBenchmark, fromHSB);
BENCHMARK_MAIN();
