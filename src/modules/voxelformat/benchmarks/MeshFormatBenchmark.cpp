/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "io/FilesystemArchive.h"
#include "scenegraph/SceneGraph.h"
#include "voxelformat/Format.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/private/mesh/FBXFormat.h"
#include "voxelformat/private/mesh/GLTFFormat.h"

class MeshFormatBenchmark : public app::AbstractBenchmark {
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

BENCHMARK_DEFINE_F(MeshFormatBenchmark, GLTF)(benchmark::State &state) {
	for (auto _ : state) {
		voxelformat::GLTFFormat f;
		const core::String filename = "glTF/lantern/Lantern.gltf";
		f.load(filename, _archive, _sceneGraph, _ctx);
		_sceneGraph.clear();
	}
}

BENCHMARK_DEFINE_F(MeshFormatBenchmark, FBX)(benchmark::State &state) {
	for (auto _ : state) {
		voxelformat::FBXFormat f;
		const core::String filename = "chr_knight.fbx";
		f.load(filename, _archive, _sceneGraph, _ctx);
		_sceneGraph.clear();
	}
}

BENCHMARK_REGISTER_F(MeshFormatBenchmark, GLTF);
BENCHMARK_REGISTER_F(MeshFormatBenchmark, FBX);

BENCHMARK_MAIN();
