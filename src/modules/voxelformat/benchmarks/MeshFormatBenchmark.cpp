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
#include "voxelformat/private/mesh/MeshFormat.h"
#include "voxelformat/private/mesh/MeshMaterial.h"

class MeshFormatBenchmark : public app::AbstractBenchmark {
private:
	using Super = app::AbstractBenchmark;

protected:
	class MeshFormatEx : public voxelformat::MeshFormat {
	private:
		using Super = voxelformat::MeshFormat;

	public:
		void voxelizePointCloud(voxelformat::PointCloud &&vertices) {
			scenegraph::SceneGraph sceneGraph;
			Super::voxelizePointCloud("benchmark", sceneGraph, core::move(vertices));
		}

		void transformTris(const voxelformat::MeshTriCollection &tris, voxelformat::PosMap &posMap, const voxelformat::MeshMaterialArray &meshMaterialArray) const {
			palette::NormalPalette normalPalette;
			normalPalette.redAlert2();
			voxel::Region region{-1000, 1000};
			Super::transformTris(region, tris, posMap, meshMaterialArray, normalPalette);
		}

		void transformTrisAxisAligned(const voxelformat::MeshTriCollection &tris, voxelformat::PosMap &posMap, const voxelformat::MeshMaterialArray &meshMaterialArray) const {
			palette::NormalPalette normalPalette;
			normalPalette.redAlert2();
			voxel::Region region{-1000, 1000};
			Super::transformTrisAxisAligned(region, tris, posMap, meshMaterialArray, normalPalette);
		}

		bool saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &, const ChunkMeshes &,
						const core::String &, const io::ArchivePtr &, const glm::vec3 &, bool, bool, bool) override {
			return false;
		}
	};

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

BENCHMARK_DEFINE_F(MeshFormatBenchmark, voxelizePointCloud)(benchmark::State &state) {
	for (auto _ : state) {
		MeshFormatEx f;
		voxelformat::PointCloud vertices;
		vertices.resize(10000);
		f.voxelizePointCloud(core::move(vertices));
	}
}

BENCHMARK_DEFINE_F(MeshFormatBenchmark, transformTris)(benchmark::State &state) {
	voxelformat::MeshTriCollection tris;
	tris.resize(10000);
	for (auto _ : state) {
		MeshFormatEx f;
		voxelformat::PosMap posMap;
		f.transformTris(tris, posMap, {});
	}
}

BENCHMARK_DEFINE_F(MeshFormatBenchmark, transformTrisAxisAligned)(benchmark::State &state) {
	voxelformat::MeshTriCollection tris;
	tris.resize(10000);
	for (auto _ : state) {
		MeshFormatEx f;
		voxelformat::PosMap posMap;
		f.transformTrisAxisAligned(tris, posMap, {});
	}
}

BENCHMARK_REGISTER_F(MeshFormatBenchmark, GLTF);
BENCHMARK_REGISTER_F(MeshFormatBenchmark, FBX);
BENCHMARK_REGISTER_F(MeshFormatBenchmark, voxelizePointCloud);
BENCHMARK_REGISTER_F(MeshFormatBenchmark, transformTris);
BENCHMARK_REGISTER_F(MeshFormatBenchmark, transformTrisAxisAligned);

BENCHMARK_MAIN();
