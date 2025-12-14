#include "app/benchmark/AbstractBenchmark.h"
#include "core/Assert.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Map.h"
#include <map>
#include <unordered_map>
#include <vector>

class MapBenchmark : public app::AbstractBenchmark {};

BENCHMARK_DEFINE_F(MapBenchmark, compareToMapStd)(benchmark::State &state) {
	std::map<int64_t, int64_t> map;
	for (auto _ : state) {
		const int64_t n = state.range(0);
		for (int64_t i = 0; i < n; ++i) {
			map.insert(std::make_pair(i, i));
			const int value = (int)map[i];
			if (value != i) {
				state.SkipWithError("Failed!");
				break;
			}
		}
	}
}

BENCHMARK_DEFINE_F(MapBenchmark, compareToUnorderedMapStd)(benchmark::State &state) {
	std::unordered_map<int64_t, int64_t, std::hash<int64_t>> unorderedMap;
	for (auto _ : state) {
		const int64_t n = state.range(0);
		unorderedMap.reserve(n);
		for (int64_t i = 0; i < n; ++i) {
			unorderedMap.insert(std::make_pair(i, i));
			const int value = (int)unorderedMap[i];
			if (value != i) {
				state.SkipWithError("Failed!");
				break;
			}
		}
	}
}

BENCHMARK_DEFINE_F(MapBenchmark, compareToMapCore)(benchmark::State &state) {
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

class DynamicArrayBenchmark : public app::AbstractBenchmark {
protected:
	struct TestData {
		core::String testStr;
	};
};

BENCHMARK_DEFINE_F(DynamicArrayBenchmark, StdVectorPushBack)(benchmark::State &state) {
	for (auto _ : state) {
		std::vector<TestData> vec;
		const int64_t n = state.range(0);
		vec.reserve(n);
		for (int64_t i = 0; i < n; ++i) {
			vec.push_back(TestData{core::String("test")});
		}
	}
}

BENCHMARK_DEFINE_F(DynamicArrayBenchmark, DynamicArrayPushBack)(benchmark::State &state) {
	for (auto _ : state) {
		core::DynamicArray<TestData> vec;
		const int64_t n = state.range(0);
		vec.reserve(n);
		for (int64_t i = 0; i < n; ++i) {
			vec.push_back(TestData{core::String("test")});
		}
	}
}

BENCHMARK_REGISTER_F(DynamicArrayBenchmark, StdVectorPushBack)->RangeMultiplier(2)->Range(8, 8 << 10);
BENCHMARK_REGISTER_F(DynamicArrayBenchmark, DynamicArrayPushBack)->RangeMultiplier(2)->Range(8, 8 << 10);

BENCHMARK_MAIN();
