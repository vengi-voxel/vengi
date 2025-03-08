/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "engine-config.h"
#include "image/Image.h"
#include "io/BufferedReadWriteStream.h"

#if USE_LIBJPEG
#define JPEG_WRITE_FUNC WriteLibJPEG
#define JPEG_READ_FUNC ReadLibJPEG
#else
#define JPEG_WRITE_FUNC WriteJPEGStbImage
#define JPEG_READ_FUNC ReadJPEGStbImage
#endif
#if USE_LIBPNG
#define PNG_WRITE_FUNC WriteLibPNG
#define PNG_READ_FUNC ReadLibPNG
#else
#define PNG_WRITE_FUNC WritePNGStbImage
#define PNG_READ_FUNC ReadPNGStbImage
#endif

class ImageBenchmark : public app::AbstractBenchmark {
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

BENCHMARK_DEFINE_F(ImageBenchmark, JPEG_WRITE_FUNC)(benchmark::State &state) {
	io::BufferedReadWriteStream stream(1024 * 1024 * 4);
	for (auto _ : state) {
		_imagePNG->writeJPEG(stream);
		stream.seek(0);
	}
}

BENCHMARK_DEFINE_F(ImageBenchmark, JPEG_READ_FUNC)(benchmark::State &state) {
	for (auto _ : state) {
		image::loadImage(_fileJPG);
	}
}

BENCHMARK_DEFINE_F(ImageBenchmark, PNG_WRITE_FUNC)(benchmark::State &state) {
	io::BufferedReadWriteStream stream(1024 * 1024 * 4);
	for (auto _ : state) {
		_imagePNG->writePNG(stream);
		stream.seek(0);
	}
}

BENCHMARK_DEFINE_F(ImageBenchmark, PNG_READ_FUNC)(benchmark::State &state) {
	for (auto _ : state) {
		image::loadImage(_filePNG);
	}
}

BENCHMARK_REGISTER_F(ImageBenchmark, JPEG_READ_FUNC);
BENCHMARK_REGISTER_F(ImageBenchmark, JPEG_WRITE_FUNC);

BENCHMARK_REGISTER_F(ImageBenchmark, PNG_READ_FUNC);
BENCHMARK_REGISTER_F(ImageBenchmark, PNG_WRITE_FUNC);

BENCHMARK_MAIN();
