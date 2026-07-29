#include "app/benchmark/AbstractBenchmark.h"
#include "core/UUID.h"
#include "core/collection/DynamicMap.h"

class UUIDBenchmark : public app::AbstractBenchmark {};

BENCHMARK_DEFINE_F(UUIDBenchmark, generate)(benchmark::State &state) {
	for (auto _ : state) {
		core::UUID u = core::UUID::generate();
		benchmark::DoNotOptimize(u);
	}
}

BENCHMARK_DEFINE_F(UUIDBenchmark, copy)(benchmark::State &state) {
	const core::UUID src = core::UUID::generate();
	for (auto _ : state) {
		core::UUID u = src;
		benchmark::DoNotOptimize(u);
	}
}

BENCHMARK_DEFINE_F(UUIDBenchmark, equal)(benchmark::State &state) {
	const core::UUID a = core::UUID::generate();
	const core::UUID b = a;
	const core::UUID c = core::UUID::generate();
	for (auto _ : state) {
		bool eq = (a == b);
		bool ne = (a == c);
		benchmark::DoNotOptimize(eq);
		benchmark::DoNotOptimize(ne);
	}
}

BENCHMARK_DEFINE_F(UUIDBenchmark, hashMapLookup)(benchmark::State &state) {
	core::DynamicMap<core::UUID, int, 251, core::UUIDHash> map;
	core::UUID keys[64];
	for (int i = 0; i < 64; ++i) {
		keys[i] = core::UUID::generate();
		map.put(keys[i], i);
	}
	for (auto _ : state) {
		int sum = 0;
		for (int i = 0; i < 64; ++i) {
			int value = 0;
			if (map.get(keys[i], value)) {
				sum += value;
			}
		}
		benchmark::DoNotOptimize(sum);
	}
}

BENCHMARK_DEFINE_F(UUIDBenchmark, str)(benchmark::State &state) {
	const core::UUID u = core::UUID::generate();
	for (auto _ : state) {
		core::String s = u.str();
		benchmark::DoNotOptimize(s);
	}
}

BENCHMARK_DEFINE_F(UUIDBenchmark, parse)(benchmark::State &state) {
	const core::String s = core::UUID::generate().str();
	for (auto _ : state) {
		core::UUID u(s);
		benchmark::DoNotOptimize(u);
	}
}

BENCHMARK_REGISTER_F(UUIDBenchmark, generate);
BENCHMARK_REGISTER_F(UUIDBenchmark, copy);
BENCHMARK_REGISTER_F(UUIDBenchmark, equal);
BENCHMARK_REGISTER_F(UUIDBenchmark, hashMapLookup);
BENCHMARK_REGISTER_F(UUIDBenchmark, str);
BENCHMARK_REGISTER_F(UUIDBenchmark, parse);
