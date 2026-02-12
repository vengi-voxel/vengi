/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "palette/Palette.h"
#include "voxel/ChunkMesh.h"
#include "voxel/RawVolume.h"
#include "voxel/SurfaceExtractor.h"

class SurfaceExtractorBenchmark : public app::AbstractBenchmark {
private:
	using Super = app::AbstractBenchmark;
protected:
	voxel::RawVolume v{voxel::Region{0, 0, 0, 61, 22, 61}};

public:
	SurfaceExtractorBenchmark() : Super(2) {
	}
	void SetUp(::benchmark::State &state) override {
		app::AbstractBenchmark::SetUp(state);

		v.setVoxel(6, 6, 52, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 6, 52, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 6, 52, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 7, 52, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 7, 52, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 7, 52, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 8, 52, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 8, 52, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 8, 52, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 6, 53, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 6, 53, voxel::createVoxel(voxel::VoxelType::Generic, 2));
		v.setVoxel(8, 6, 53, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 7, 53, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 7, 53, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 7, 53, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 8, 53, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 8, 53, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 8, 53, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(9, 5, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(5, 6, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 6, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 6, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 6, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(9, 6, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 7, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 7, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 7, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 8, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 8, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 8, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(9, 5, 55, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(5, 6, 55, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 6, 55, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 6, 55, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 6, 55, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(9, 6, 55, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 7, 55, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 7, 55, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 7, 55, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(5, 5, 56, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 5, 56, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 5, 56, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 5, 56, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(9, 5, 56, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(5, 6, 56, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 6, 56, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 6, 56, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 6, 56, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(9, 6, 56, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(5, 5, 57, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 5, 57, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 5, 57, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 5, 57, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(5, 6, 57, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 6, 57, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 6, 57, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 6, 57, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(5, 5, 58, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 5, 58, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 5, 58, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 5, 58, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(5, 6, 58, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 6, 58, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 6, 58, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 6, 58, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(5, 5, 59, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 5, 59, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 5, 59, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 5, 59, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(5, 6, 59, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 6, 59, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 6, 59, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 6, 59, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
};

BENCHMARK_DEFINE_F(SurfaceExtractorBenchmark, Cubic)(benchmark::State &state) {
	for (auto _ : state) {
		const bool mergeQuads = true;
		const bool reuseVertices = true;
		const bool ambientOcclusion = false;

		voxel::ChunkMesh mesh{65536, 65536, false};

		voxel::SurfaceExtractionContext ctx =
			voxel::buildCubicContext(&v, v.region(), mesh, glm::ivec3(0), mergeQuads, reuseVertices, ambientOcclusion);
		voxel::extractSurface(ctx);
	}
}

BENCHMARK_DEFINE_F(SurfaceExtractorBenchmark, Binary)(benchmark::State &state) {
	for (auto _ : state) {
		const bool ambientOcclusion = false;

		voxel::ChunkMesh mesh{65536, 65536, false};

		voxel::SurfaceExtractionContext ctx =
			voxel::buildBinaryContext(&v, v.region(), mesh, glm::ivec3(0), ambientOcclusion);
		voxel::extractSurface(ctx);
	}
}

BENCHMARK_DEFINE_F(SurfaceExtractorBenchmark, MarchingCubes)(benchmark::State &state) {
	palette::Palette pal;
	pal.nippon();
	for (auto _ : state) {
		voxel::ChunkMesh mesh{65536, 65536, false};

		voxel::SurfaceExtractionContext ctx =
			voxel::buildMarchingCubesContext(&v, v.region(), mesh, pal);
		voxel::extractSurface(ctx);
	}
}

BENCHMARK_REGISTER_F(SurfaceExtractorBenchmark, Cubic);
BENCHMARK_REGISTER_F(SurfaceExtractorBenchmark, Binary);
BENCHMARK_REGISTER_F(SurfaceExtractorBenchmark, MarchingCubes);

BENCHMARK_MAIN();
