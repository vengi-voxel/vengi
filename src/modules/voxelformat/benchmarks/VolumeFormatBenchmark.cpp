/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "io/FilesystemArchive.h"
#include "scenegraph/SceneGraph.h"
#include "voxelformat/Format.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/private/goxel/GoxFormat.h"
#include "voxelformat/private/qubicle/QBCLFormat.h"
#include "voxelformat/private/qubicle/QBFormat.h"
#include "voxelformat/private/vengi/VENGIFormat.h"

class VolumeFormatBenchmark : public app::AbstractBenchmark {
private:
	using Super = app::AbstractBenchmark;

protected:
	voxelformat::LoadContext _ctx;
	scenegraph::SceneGraph _sceneGraph;
	io::ArchivePtr _archive;
	void SetUp(::benchmark::State &state) override {
		Super::SetUp(state);
		_archive = io::openFilesystemArchive(_benchmarkApp->filesystem());
		voxelformat::FormatConfig::init();
	}
};

BENCHMARK_DEFINE_F(VolumeFormatBenchmark, QB)(benchmark::State &state) {
	for (auto _ : state) {
		voxelformat::QBFormat f;
		const core::String filename = "chr_knight.qb";
		f.load(filename, _archive, _sceneGraph, _ctx);
		_sceneGraph.clear();
	}
}

BENCHMARK_DEFINE_F(VolumeFormatBenchmark, QBCL)(benchmark::State &state) {
	for (auto _ : state) {
		voxelformat::QBCLFormat f;
		const core::String filename = "chr_knight.qbcl";
		f.load(filename, _archive, _sceneGraph, _ctx);
		_sceneGraph.clear();
	}
}

BENCHMARK_DEFINE_F(VolumeFormatBenchmark, GOX)(benchmark::State &state) {
	for (auto _ : state) {
		voxelformat::GoxFormat f;
		const core::String filename = "chr_knight.gox";
		f.load(filename, _archive, _sceneGraph, _ctx);
		_sceneGraph.clear();
	}
}

BENCHMARK_DEFINE_F(VolumeFormatBenchmark, VENGI)(benchmark::State &state) {
	for (auto _ : state) {
		voxelformat::VENGIFormat f;
		const core::String filename = "chr_knight.vengi";
		f.load(filename, _archive, _sceneGraph, _ctx);
		_sceneGraph.clear();
	}
}

BENCHMARK_REGISTER_F(VolumeFormatBenchmark, QB);
BENCHMARK_REGISTER_F(VolumeFormatBenchmark, QBCL);
BENCHMARK_REGISTER_F(VolumeFormatBenchmark, GOX);
BENCHMARK_REGISTER_F(VolumeFormatBenchmark, VENGI);
