/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "image/Image.h"
#include "io/BufferedReadWriteStream.h"

class ImageWriteBenchmark : public app::AbstractBenchmark {
protected:
	image::ImagePtr _image;

public:
	void SetUp(::benchmark::State &state) override {
		app::AbstractBenchmark::SetUp(state);
		io::FilePtr file = _benchmarkApp->filesystem()->open("test-palette-in.png");
		_image = image::loadImage(file);
	}
};

BENCHMARK_DEFINE_F(ImageWriteBenchmark, Visit)(benchmark::State &state) {
	io::BufferedReadWriteStream stream(1024 * 1024 * 4);
	for (auto _ : state) {
		_image->writeJPEG(stream);
		stream.seek(0);
	}
}

BENCHMARK_REGISTER_F(ImageWriteBenchmark, Visit);

BENCHMARK_MAIN();
