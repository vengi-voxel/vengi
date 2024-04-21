/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "voxel/ChunkMesh.h"
#include "voxel/RawVolume.h"
#include "voxel/SurfaceExtractor.h"

class SurfaceExtractorBenchmark : public app::AbstractBenchmark {
protected:
	voxel::RawVolume v{voxel::Region{0, 0, 0, 143, 22, 134}};

public:
	void SetUp(::benchmark::State &state) override {
		app::AbstractBenchmark::SetUp(state);

		v.setVoxel(96, 6, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(97, 6, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(98, 6, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 7, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(97, 7, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(98, 7, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 8, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(97, 8, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(98, 8, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 6, 63, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(97, 6, 63, voxel::createVoxel(voxel::VoxelType::Generic, 2));
		v.setVoxel(98, 6, 63, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 7, 63, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(97, 7, 63, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(98, 7, 63, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 8, 63, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(97, 8, 63, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(98, 8, 63, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(99, 5, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(95, 6, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 6, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(97, 6, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(98, 6, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(99, 6, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 7, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(97, 7, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(98, 7, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 8, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(97, 8, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(98, 8, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(99, 5, 65, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(95, 6, 65, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 6, 65, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(97, 6, 65, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(98, 6, 65, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(99, 6, 65, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 7, 65, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(97, 7, 65, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(98, 7, 65, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(95, 5, 66, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 5, 66, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(97, 5, 66, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(98, 5, 66, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(99, 5, 66, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(95, 6, 66, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 6, 66, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(97, 6, 66, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(98, 6, 66, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(99, 6, 66, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(95, 5, 67, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 5, 67, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(97, 5, 67, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(98, 5, 67, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(95, 6, 67, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 6, 67, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(97, 6, 67, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(98, 6, 67, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(95, 5, 68, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 5, 68, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(97, 5, 68, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(98, 5, 68, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(95, 6, 68, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 6, 68, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(97, 6, 68, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(98, 6, 68, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(95, 5, 69, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 5, 69, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(97, 5, 69, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(98, 5, 69, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(95, 6, 69, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 6, 69, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(97, 6, 69, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(98, 6, 69, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
};

BENCHMARK_DEFINE_F(SurfaceExtractorBenchmark, Visit)(benchmark::State &state) {
	for (auto _ : state) {
		const bool mergeQuads = true;
		const bool reuseVertices = true;
		const bool ambientOcclusion = false;

		voxel::ChunkMesh mesh;

		voxel::SurfaceExtractionContext ctx =
			voxel::buildCubicContext(&v, v.region(), mesh, glm::ivec3(0), mergeQuads, reuseVertices, ambientOcclusion);
		voxel::extractSurface(ctx);
	}
}

BENCHMARK_REGISTER_F(SurfaceExtractorBenchmark, Visit);

BENCHMARK_MAIN();
