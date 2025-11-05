/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "core/collection/DynamicArray.h"
#include "io/BufferedReadWriteStream.h"
#include "io/ZipReadStream.h"
#include "io/ZipWriteStream.h"

class StreamBenchmark : public app::AbstractBenchmark {
protected:
	using Super = app::AbstractBenchmark;

	core::DynamicArray<uint32_t> data;

public:
	StreamBenchmark() {
		data.reserve(1024 * 1024 * 3);
	}

	template<typename WriteStreamType, typename ReadStreamType>
	void compress() {
		io::BufferedReadWriteStream outStream(data.capacity());
		WriteStreamType stream(outStream);
		stream.write(data.data(), data.capacity());
		stream.flush();
		outStream.seek(0);
		Log::error("Compressed size: %d bytes", (int)outStream.size());
		ReadStreamType inStream(outStream, (int)outStream.size());
		core::DynamicArray<uint32_t> decompressed;
		decompressed.resize(data.capacity());
		inStream.read(decompressed.data(), decompressed.size() * sizeof(uint32_t));
	}
};

BENCHMARK_DEFINE_F(StreamBenchmark, ZipStream)(benchmark::State &state) {
	for (auto _ : state) {
		compress<io::ZipWriteStream, io::ZipReadStream>();
	}
}

BENCHMARK_REGISTER_F(StreamBenchmark, ZipStream);
BENCHMARK_MAIN();
