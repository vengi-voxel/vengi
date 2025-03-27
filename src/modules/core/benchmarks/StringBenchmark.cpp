#include "app/benchmark/AbstractBenchmark.h"
#include "core/StringUtil.h"

class StringBenchmark: public app::AbstractBenchmark {
};

BENCHMARK_DEFINE_F(StringBenchmark, ctor1) (benchmark::State& state) {
	for (auto _ : state) {
		core::String str("test");
		benchmark::DoNotOptimize(str);
	}
}

BENCHMARK_DEFINE_F(StringBenchmark, ctor2) (benchmark::State& state) {
	for (auto _ : state) {
		core::String str("test", 4);
		benchmark::DoNotOptimize(str);
	}
}

BENCHMARK_DEFINE_F(StringBenchmark, ctor3) (benchmark::State& state) {
	for (auto _ : state) {
		core::String str("");
		benchmark::DoNotOptimize(str);
	}
}

BENCHMARK_DEFINE_F(StringBenchmark, format) (benchmark::State& state) {
	for (auto _ : state) {
		core::String str(core::string::format("%100s", "test"));
		benchmark::DoNotOptimize(str);
	}
}

BENCHMARK_DEFINE_F(StringBenchmark, formatBuf) (benchmark::State& state) {
	for (auto _ : state) {
		char buf[101];
		core::string::formatBuf(buf, sizeof(buf), "%100s", "test");
	}
}

BENCHMARK_DEFINE_F(StringBenchmark, formatStr) (benchmark::State& state) {
	for (auto _ : state) {
		core::String str(core::String::format("%100s", "test"));
		benchmark::DoNotOptimize(str);
	}
}

BENCHMARK_DEFINE_F(StringBenchmark, stringConcat) (benchmark::State& state) {
	for (auto _ : state) {
		core::String str("test");
		str += "test";
		benchmark::DoNotOptimize(str);
	}
}

BENCHMARK_DEFINE_F(StringBenchmark, stringConcatViaFormat) (benchmark::State& state) {
	for (auto _ : state) {
		core::String str(core::String::format("test%s", "test"));
		benchmark::DoNotOptimize(str);
	}
}

BENCHMARK_REGISTER_F(StringBenchmark, format);
BENCHMARK_REGISTER_F(StringBenchmark, formatBuf);
BENCHMARK_REGISTER_F(StringBenchmark, formatStr);
BENCHMARK_REGISTER_F(StringBenchmark, stringConcat);
BENCHMARK_REGISTER_F(StringBenchmark, stringConcatViaFormat);
BENCHMARK_REGISTER_F(StringBenchmark, ctor1);
BENCHMARK_REGISTER_F(StringBenchmark, ctor2);
BENCHMARK_REGISTER_F(StringBenchmark, ctor3);
