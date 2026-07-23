/**
 * @file
 */

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
		benchmark::DoNotOptimize(color::getRGBA({0.0f, 0.0f, 0.0f, 0.0f}));
	}
}

BENCHMARK_DEFINE_F(ColorUtilBenchmark, getRGBA3)(benchmark::State &state) {
	for (auto _ : state) {
		benchmark::DoNotOptimize(color::getRGB({0.0f, 0.0f, 0.0f}));
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

BENCHMARK_DEFINE_F(ColorUtilBenchmark, getDistanceApprox)(benchmark::State &state) {
	const color::RGBA a(13, 39, 26, 255);
	const color::RGBA b(200, 10, 90, 255);
	for (auto _ : state) {
		benchmark::DoNotOptimize(color::getDistance(a, b, color::Distance::Approximation));
	}
}

BENCHMARK_DEFINE_F(ColorUtilBenchmark, getDistanceHSB)(benchmark::State &state) {
	const color::RGBA a(13, 39, 26, 255);
	const color::RGBA b(200, 10, 90, 255);
	for (auto _ : state) {
		benchmark::DoNotOptimize(color::getDistance(a, b, color::Distance::HSB));
	}
}

BENCHMARK_DEFINE_F(ColorUtilBenchmark, rgbaMix)(benchmark::State &state) {
	const color::RGBA a(13, 39, 26, 255);
	const color::RGBA b(200, 10, 90, 128);
	for (auto _ : state) {
		benchmark::DoNotOptimize(color::RGBA::mix(a, b, 0.35f));
	}
}

BENCHMARK_DEFINE_F(ColorUtilBenchmark, rgbaBrightness)(benchmark::State &state) {
	const color::RGBA a(13, 39, 26, 255);
	for (auto _ : state) {
		benchmark::DoNotOptimize(a.brightness());
	}
}

BENCHMARK_REGISTER_F(ColorUtilBenchmark, fromRGBA);
BENCHMARK_REGISTER_F(ColorUtilBenchmark, fromRGBA2);
BENCHMARK_REGISTER_F(ColorUtilBenchmark, getRGBA4);
BENCHMARK_REGISTER_F(ColorUtilBenchmark, getRGBA3);
BENCHMARK_REGISTER_F(ColorUtilBenchmark, getHSB);
BENCHMARK_REGISTER_F(ColorUtilBenchmark, fromHSB);
BENCHMARK_REGISTER_F(ColorUtilBenchmark, getDistanceApprox);
BENCHMARK_REGISTER_F(ColorUtilBenchmark, getDistanceHSB);
BENCHMARK_REGISTER_F(ColorUtilBenchmark, rgbaMix);
BENCHMARK_REGISTER_F(ColorUtilBenchmark, rgbaBrightness);
BENCHMARK_MAIN();
