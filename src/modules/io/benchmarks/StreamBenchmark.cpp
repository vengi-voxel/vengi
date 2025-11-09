/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "core/collection/DynamicArray.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Base64ReadStream.h"
#include "io/Base64WriteStream.h"
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
	void writeRead() {
		io::BufferedReadWriteStream outStream;
		{
			// some streams needs a flush that is called by the dtor
			WriteStreamType stream(outStream);
			stream.write(data.data(), data.capacity());
		}
		outStream.seek(0);
		ReadStreamType inStream(outStream);
		core::DynamicArray<uint32_t> decompressed;
		decompressed.resize(data.capacity());
		inStream.read(decompressed.data(), decompressed.size() * sizeof(uint32_t));
	}

	template<typename WriteStreamType, typename ReadStreamType>
	void write() {
		io::BufferedReadWriteStream outStream;
		WriteStreamType stream(outStream);
		stream.write(data.data(), data.capacity());
	}
};

BENCHMARK_DEFINE_F(StreamBenchmark, ZipStreamRoundTrip)(benchmark::State &state) {
	for (auto _ : state) {
		writeRead<io::ZipWriteStream, io::ZipReadStream>();
	}
}

BENCHMARK_DEFINE_F(StreamBenchmark, Base64StreamRoundTrip)(benchmark::State &state) {
	for (auto _ : state) {
		writeRead<io::Base64WriteStream, io::Base64ReadStream>();
	}
}

BENCHMARK_DEFINE_F(StreamBenchmark, ZipStreamWrite)(benchmark::State &state) {
	for (auto _ : state) {
		write<io::ZipWriteStream, io::ZipReadStream>();
	}
}

BENCHMARK_DEFINE_F(StreamBenchmark, Base64StreamWrite)(benchmark::State &state) {
	for (auto _ : state) {
		write<io::Base64WriteStream, io::Base64ReadStream>();
	}
}

BENCHMARK_REGISTER_F(StreamBenchmark, ZipStreamRoundTrip);
BENCHMARK_REGISTER_F(StreamBenchmark, Base64StreamRoundTrip);
BENCHMARK_REGISTER_F(StreamBenchmark, ZipStreamWrite);
BENCHMARK_REGISTER_F(StreamBenchmark, Base64StreamWrite);
BENCHMARK_MAIN();
