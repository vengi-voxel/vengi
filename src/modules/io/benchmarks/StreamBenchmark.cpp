/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "core/collection/DynamicArray.h"
#include "io/Base64ReadStream.h"
#include "io/Base64WriteStream.h"
#include "io/BufferedReadWriteStream.h"
#include "io/StdStreamBuf.h"
#include "io/ZipReadStream.h"
#include "io/ZipWriteStream.h"
#include <istream>
#include <ostream>

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

BENCHMARK_DEFINE_F(StreamBenchmark, Base64StreamRead)(benchmark::State &state) {
	io::BufferedReadWriteStream outStream;
	{
		io::Base64WriteStream stream(outStream);
		stream.write(data.data(), data.capacity());
	}
	core::DynamicArray<uint32_t> decompressed;
	decompressed.resize(data.capacity());
	for (auto _ : state) {
		outStream.seek(0);
		io::Base64ReadStream inStream(outStream);
		inStream.read(decompressed.data(), decompressed.size() * sizeof(uint32_t));
	}
}

BENCHMARK_DEFINE_F(StreamBenchmark, BufferedStream)(benchmark::State &state) {
	for (auto _ : state) {
		io::BufferedReadWriteStream stream;
		stream.reserve(1024);
		for (int i = 0; i < 1024; ++i) {
			stream.writeUInt8(1);
		}
		benchmark::DoNotOptimize(stream.pos());
	}
}

BENCHMARK_REGISTER_F(StreamBenchmark, ZipStreamRoundTrip);
BENCHMARK_REGISTER_F(StreamBenchmark, Base64StreamRoundTrip);
BENCHMARK_REGISTER_F(StreamBenchmark, ZipStreamWrite);
BENCHMARK_REGISTER_F(StreamBenchmark, Base64StreamWrite);
BENCHMARK_REGISTER_F(StreamBenchmark, Base64StreamRead);
BENCHMARK_REGISTER_F(StreamBenchmark, BufferedStream);

BENCHMARK_DEFINE_F(StreamBenchmark, StdOStreamBufWrite)(benchmark::State &state) {
	for (auto _ : state) {
		io::BufferedReadWriteStream target;
		target.reserve(1024);
		io::StdOStreamBuf buf(target);
		std::ostream ostream(&buf);
		for (int i = 0; i < 1024; ++i) {
			ostream.put((char)1);
		}
		ostream.flush();
		benchmark::DoNotOptimize(target.pos());
	}
}

BENCHMARK_DEFINE_F(StreamBenchmark, StdOStreamBufBulkWrite)(benchmark::State &state) {
	const char block[1024] = {};
	for (auto _ : state) {
		io::BufferedReadWriteStream target;
		target.reserve(sizeof(block));
		io::StdOStreamBuf buf(target);
		std::ostream ostream(&buf);
		ostream.write(block, sizeof(block));
		ostream.flush();
		benchmark::DoNotOptimize(target.pos());
	}
}

BENCHMARK_DEFINE_F(StreamBenchmark, StdIStreamBufRead)(benchmark::State &state) {
	io::BufferedReadWriteStream source;
	source.reserve(1024);
	for (int i = 0; i < 1024; ++i) {
		source.writeUInt8(1);
	}
	for (auto _ : state) {
		source.seek(0);
		io::StdIStreamBuf buf(source);
		std::istream istream(&buf);
		char c;
		int count = 0;
		while (istream.get(c)) {
			++count;
		}
		benchmark::DoNotOptimize(count);
	}
}

BENCHMARK_REGISTER_F(StreamBenchmark, StdOStreamBufWrite);
BENCHMARK_REGISTER_F(StreamBenchmark, StdOStreamBufBulkWrite);
BENCHMARK_REGISTER_F(StreamBenchmark, StdIStreamBufRead);

BENCHMARK_MAIN();
