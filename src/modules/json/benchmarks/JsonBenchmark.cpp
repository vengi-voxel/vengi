/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "json/JSON.h"

class JsonBenchmark : public app::AbstractBenchmark {};

static const char *smallJson = R"({"name":"test","value":42,"active":true})";

static const char *mediumJson = R"({
	"users": [
		{"id": 1, "name": "Alice", "score": 95.5, "active": true},
		{"id": 2, "name": "Bob", "score": 82.3, "active": false},
		{"id": 3, "name": "Charlie", "score": 91.0, "active": true},
		{"id": 4, "name": "Diana", "score": 88.7, "active": true},
		{"id": 5, "name": "Eve", "score": 76.1, "active": false}
	],
	"metadata": {
		"total": 5,
		"page": 1,
		"perPage": 10,
		"query": "all"
	}
})";

static core::String generateLargeJson(int numItems) {
	core::String s = "{\"items\":[";
	for (int i = 0; i < numItems; ++i) {
		if (i > 0) {
			s += ",";
		}
		s += core::String::format(
			"{\"id\":%d,\"name\":\"item_%d\",\"x\":%d.%d,\"y\":%d.%d,\"z\":%d.%d,\"active\":%s}",
			i, i, i * 3, i % 10, i * 7, i % 10, i * 11, i % 10, (i % 2 == 0) ? "true" : "false");
	}
	s += "]}";
	return s;
}

BENCHMARK_DEFINE_F(JsonBenchmark, parseSmall)(benchmark::State &state) {
	for (auto _ : state) {
		json::Json j = json::Json::parse(smallJson);
		benchmark::DoNotOptimize(j);
	}
}
BENCHMARK_REGISTER_F(JsonBenchmark, parseSmall);

BENCHMARK_DEFINE_F(JsonBenchmark, parseMedium)(benchmark::State &state) {
	for (auto _ : state) {
		json::Json j = json::Json::parse(mediumJson);
		benchmark::DoNotOptimize(j);
	}
}
BENCHMARK_REGISTER_F(JsonBenchmark, parseMedium);

BENCHMARK_DEFINE_F(JsonBenchmark, parseLarge)(benchmark::State &state) {
	const core::String largeJson = generateLargeJson((int)state.range(0));
	for (auto _ : state) {
		json::Json j = json::Json::parse(largeJson);
		benchmark::DoNotOptimize(j);
	}
}
BENCHMARK_REGISTER_F(JsonBenchmark, parseLarge)->RangeMultiplier(10)->Range(10, 10000);

BENCHMARK_DEFINE_F(JsonBenchmark, accept)(benchmark::State &state) {
	for (auto _ : state) {
		bool valid = json::Json::accept(mediumJson);
		benchmark::DoNotOptimize(valid);
	}
}
BENCHMARK_REGISTER_F(JsonBenchmark, accept);

BENCHMARK_DEFINE_F(JsonBenchmark, dumpSmall)(benchmark::State &state) {
	json::Json j = json::Json::parse(smallJson);
	for (auto _ : state) {
		core::String s = j.dump();
		benchmark::DoNotOptimize(s);
	}
}
BENCHMARK_REGISTER_F(JsonBenchmark, dumpSmall);

BENCHMARK_DEFINE_F(JsonBenchmark, dumpMedium)(benchmark::State &state) {
	json::Json j = json::Json::parse(mediumJson);
	for (auto _ : state) {
		core::String s = j.dump();
		benchmark::DoNotOptimize(s);
	}
}
BENCHMARK_REGISTER_F(JsonBenchmark, dumpMedium);

BENCHMARK_DEFINE_F(JsonBenchmark, dumpFormatted)(benchmark::State &state) {
	json::Json j = json::Json::parse(mediumJson);
	for (auto _ : state) {
		core::String s = j.dump(2);
		benchmark::DoNotOptimize(s);
	}
}
BENCHMARK_REGISTER_F(JsonBenchmark, dumpFormatted);

BENCHMARK_DEFINE_F(JsonBenchmark, getByKey)(benchmark::State &state) {
	json::Json j = json::Json::parse(mediumJson);
	for (auto _ : state) {
		json::Json users = j.get("users");
		json::Json meta = j.get("metadata");
		benchmark::DoNotOptimize(users);
		benchmark::DoNotOptimize(meta);
	}
}
BENCHMARK_REGISTER_F(JsonBenchmark, getByKey);

BENCHMARK_DEFINE_F(JsonBenchmark, getByIndex)(benchmark::State &state) {
	json::Json j = json::Json::parse(mediumJson);
	json::Json users = j.get("users");
	const int n = users.size();
	for (auto _ : state) {
		for (int i = 0; i < n; ++i) {
			json::Json item = users.get(i);
			benchmark::DoNotOptimize(item);
		}
	}
}
BENCHMARK_REGISTER_F(JsonBenchmark, getByIndex);

BENCHMARK_DEFINE_F(JsonBenchmark, contains)(benchmark::State &state) {
	json::Json j = json::Json::parse(mediumJson);
	for (auto _ : state) {
		bool a = j.contains("users");
		bool b = j.contains("metadata");
		bool c = j.contains("nonexistent");
		benchmark::DoNotOptimize(a);
		benchmark::DoNotOptimize(b);
		benchmark::DoNotOptimize(c);
	}
}
BENCHMARK_REGISTER_F(JsonBenchmark, contains);

BENCHMARK_DEFINE_F(JsonBenchmark, readValues)(benchmark::State &state) {
	json::Json j = json::Json::parse(mediumJson);
	json::Json user = j.get("users").get(0);
	for (auto _ : state) {
		int id = user.intVal("id", 0);
		core::String name = user.strVal("name", "");
		double score = user.doubleVal("score", 0.0);
		bool active = user.boolVal("active", false);
		benchmark::DoNotOptimize(id);
		benchmark::DoNotOptimize(name);
		benchmark::DoNotOptimize(score);
		benchmark::DoNotOptimize(active);
	}
}
BENCHMARK_REGISTER_F(JsonBenchmark, readValues);

BENCHMARK_DEFINE_F(JsonBenchmark, iterate)(benchmark::State &state) {
	json::Json j = json::Json::parse(mediumJson);
	json::Json users = j.get("users");
	for (auto _ : state) {
		int sum = 0;
		for (const json::Json &user : users) {
			sum += user.intVal("id", 0);
		}
		benchmark::DoNotOptimize(sum);
	}
}
BENCHMARK_REGISTER_F(JsonBenchmark, iterate);

BENCHMARK_DEFINE_F(JsonBenchmark, iterateKeys)(benchmark::State &state) {
	json::Json j = json::Json::parse(mediumJson);
	json::Json meta = j.get("metadata");
	for (auto _ : state) {
		int count = 0;
		for (auto it = meta.begin(); it != meta.end(); ++it) {
			core::String k = it.key();
			benchmark::DoNotOptimize(k);
			++count;
		}
		benchmark::DoNotOptimize(count);
	}
}
BENCHMARK_REGISTER_F(JsonBenchmark, iterateKeys);

BENCHMARK_DEFINE_F(JsonBenchmark, buildObject)(benchmark::State &state) {
	for (auto _ : state) {
		json::Json obj = json::Json::object();
		obj.set("name", "benchmark");
		obj.set("id", 42);
		obj.set("score", 99.5);
		obj.set("active", true);
		benchmark::DoNotOptimize(obj);
	}
}
BENCHMARK_REGISTER_F(JsonBenchmark, buildObject);

BENCHMARK_DEFINE_F(JsonBenchmark, buildArray)(benchmark::State &state) {
	for (auto _ : state) {
		json::Json arr = json::Json::array();
		for (int i = 0; i < (int)state.range(0); ++i) {
			arr.push(i);
		}
		benchmark::DoNotOptimize(arr);
	}
}
BENCHMARK_REGISTER_F(JsonBenchmark, buildArray)->RangeMultiplier(10)->Range(10, 10000);

BENCHMARK_DEFINE_F(JsonBenchmark, buildNested)(benchmark::State &state) {
	for (auto _ : state) {
		json::Json root = json::Json::object();
		json::Json arr = json::Json::array();
		for (int i = 0; i < 20; ++i) {
			json::Json item = json::Json::object();
			item.set("id", i);
			item.set("name", "test");
			item.set("value", (double)i * 1.5);
			arr.push(item);
		}
		root.set("items", arr);
		root.set("count", 20);
		benchmark::DoNotOptimize(root);
	}
}
BENCHMARK_REGISTER_F(JsonBenchmark, buildNested);

BENCHMARK_DEFINE_F(JsonBenchmark, copySmall)(benchmark::State &state) {
	json::Json j = json::Json::parse(smallJson);
	for (auto _ : state) {
		json::Json copy = j;
		benchmark::DoNotOptimize(copy);
	}
}
BENCHMARK_REGISTER_F(JsonBenchmark, copySmall);

BENCHMARK_DEFINE_F(JsonBenchmark, copyMedium)(benchmark::State &state) {
	json::Json j = json::Json::parse(mediumJson);
	for (auto _ : state) {
		json::Json copy = j;
		benchmark::DoNotOptimize(copy);
	}
}
BENCHMARK_REGISTER_F(JsonBenchmark, copyMedium);

BENCHMARK_DEFINE_F(JsonBenchmark, roundTrip)(benchmark::State &state) {
	for (auto _ : state) {
		json::Json j = json::Json::parse(mediumJson);
		core::String s = j.dump();
		json::Json j2 = json::Json::parse(s);
		benchmark::DoNotOptimize(j2);
	}
}
BENCHMARK_REGISTER_F(JsonBenchmark, roundTrip);

BENCHMARK_MAIN();
