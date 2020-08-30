#include "app/benchmark/AbstractBenchmark.h"
#include "core/collection/Map.h"
#include "core/Assert.h"
#include <unordered_map>
#include <map>

class MapBenchmark: public core::AbstractBenchmark {
};

BENCHMARK_DEFINE_F(MapBenchmark, compareToMapStd) (benchmark::State& state) {
	std::map<int64_t, int64_t> map;
	for (auto _ : state) {
		const int64_t n = state.range(0);
		for (int64_t i = 0; i < n; ++i) {
			map.insert(std::make_pair(i, i));
			const int value = map[i];
			if (value != i) {
				state.SkipWithError("Failed!");
				break;
			}
		}
	}
}

BENCHMARK_DEFINE_F(MapBenchmark, compareToUnorderedMapStd) (benchmark::State& state) {
	std::unordered_map<int64_t, int64_t, std::hash<int64_t>> unorderedMap;
	for (auto _ : state) {
		const int64_t n = state.range(0);
		for (int64_t i = 0; i < n; ++i) {
			unorderedMap.insert(std::make_pair(i, i));
			const int value = unorderedMap[i];
			if (value != i) {
				state.SkipWithError("Failed!");
				break;
			}
		}
	}
}

BENCHMARK_DEFINE_F(MapBenchmark, compareToMapCore) (benchmark::State& state) {
	core::Map<int64_t, int64_t, 4096, std::hash<int64_t>> map;
	for (auto _ : state) {
		const int64_t n = state.range(0);
		for (int64_t i = 0; i < n; ++i) {
			map.put(i, i);
			int64_t value;
			const bool found = map.get(i, value);
			if (!found || value != i) {
				state.SkipWithError("Failed!");
				break;
			}
		}
	}
}

BENCHMARK_REGISTER_F(MapBenchmark, compareToMapCore)->RangeMultiplier(2)->Range(8, 512);
BENCHMARK_REGISTER_F(MapBenchmark, compareToMapStd)->RangeMultiplier(2)->Range(8, 512);
BENCHMARK_REGISTER_F(MapBenchmark, compareToUnorderedMapStd)->RangeMultiplier(2)->Range(8, 512);

BENCHMARK_MAIN();
