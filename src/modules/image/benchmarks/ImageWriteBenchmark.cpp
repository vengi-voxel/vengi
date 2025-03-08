/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "image/Image.h"
#include "io/BufferedReadWriteStream.h"

class ImageWriteBenchmark : public app::AbstractBenchmark {
protected:
	image::ImagePtr _imagePNG;
	io::FilePtr _filePNG;
	io::FilePtr _fileJPG;

public:
	void SetUp(::benchmark::State &state) override {
		app::AbstractBenchmark::SetUp(state);

		_filePNG = {};
		_fileJPG = {};

		_filePNG = _benchmarkApp->filesystem()->open("test-palette-in.png");
		_fileJPG = _benchmarkApp->filesystem()->open("benchmark-caveexpress.jpg");
		_imagePNG = image::loadImage(_filePNG);
	}

	void TearDown(::benchmark::State &state) override {
		_imagePNG = {};
		_filePNG->close();
		_fileJPG->close();
		_filePNG = {};
		_fileJPG = {};
		app::AbstractBenchmark::TearDown(state);
	}
};

BENCHMARK_DEFINE_F(ImageWriteBenchmark, WriteJPG)(benchmark::State &state) {
	io::BufferedReadWriteStream stream(1024 * 1024 * 4);
	for (auto _ : state) {
		_imagePNG->writeJPEG(stream);
		stream.seek(0);
	}
}

BENCHMARK_DEFINE_F(ImageWriteBenchmark, LoadJPG)(benchmark::State &state) {
	for (auto _ : state) {
		image::loadImage(_fileJPG);
	}
}

BENCHMARK_DEFINE_F(ImageWriteBenchmark, WritePNG)(benchmark::State &state) {
	io::BufferedReadWriteStream stream(1024 * 1024 * 4);
	for (auto _ : state) {
		_imagePNG->writePNG(stream);
		stream.seek(0);
	}
}

BENCHMARK_DEFINE_F(ImageWriteBenchmark, LoadPNG)(benchmark::State &state) {
	for (auto _ : state) {
		image::loadImage(_filePNG);
	}
}

BENCHMARK_REGISTER_F(ImageWriteBenchmark, LoadPNG);
BENCHMARK_REGISTER_F(ImageWriteBenchmark, LoadJPG);
BENCHMARK_REGISTER_F(ImageWriteBenchmark, WriteJPG);
BENCHMARK_REGISTER_F(ImageWriteBenchmark, WritePNG);

BENCHMARK_MAIN();
