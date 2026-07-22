/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "palette/Palette.h"
#include "voxel/ChunkMesh.h"
#include "voxel/RawVolume.h"
#include "voxel/SurfaceExtractor.h"
#include "voxelformat/FormatConfig.h"

class SurfaceExtractorBenchmark : public app::AbstractBenchmark {
private:
	using Super = app::AbstractBenchmark;

protected:
	// Sparse volume matching an older real-world sample shape
	voxel::RawVolume v{voxel::Region{0, 0, 0, 61, 22, 61}};
	// Dense solid cube - stresses face generation and quad merging
	voxel::RawVolume solid{voxel::Region{0, 0, 0, 31, 31, 31}};
	// Checkerboard - worst case for greedy merging / many unique faces
	voxel::RawVolume checker{voxel::Region{0, 0, 0, 31, 31, 31}};

public:
	SurfaceExtractorBenchmark() : Super(2) {
	}

	void SetUp(::benchmark::State &state) override {
		app::AbstractBenchmark::SetUp(state);
		voxelformat::FormatConfig::init();

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

		const voxel::Voxel solidVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
		solid.fill(solidVoxel);

		for (int z = 0; z <= 31; ++z) {
			for (int y = 0; y <= 31; ++y) {
				for (int x = 0; x <= 31; ++x) {
					if (((x + y + z) & 1) == 0) {
						checker.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, (x + y) & 15));
					}
				}
			}
		}
	}
};

BENCHMARK_DEFINE_F(SurfaceExtractorBenchmark, CubicSparse)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::ChunkMesh mesh{65536, 65536, false};
		voxel::SurfaceExtractionContext ctx =
			voxel::buildCubicContext(&v, v.region(), mesh, glm::ivec3(0), true, true, false);
		voxel::extractSurface(ctx);
	}
}

BENCHMARK_DEFINE_F(SurfaceExtractorBenchmark, BinarySparse)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::ChunkMesh mesh{65536, 65536, false};
		voxel::SurfaceExtractionContext ctx =
			voxel::buildBinaryContext(&v, v.region(), mesh, glm::ivec3(0), false);
		voxel::extractSurface(ctx);
	}
}

BENCHMARK_DEFINE_F(SurfaceExtractorBenchmark, MarchingCubesSparse)(benchmark::State &state) {
	palette::Palette pal;
	pal.nippon();
	for (auto _ : state) {
		voxel::ChunkMesh mesh{65536, 65536, false};
		voxel::SurfaceExtractionContext ctx = voxel::buildMarchingCubesContext(&v, v.region(), mesh, pal);
		voxel::extractSurface(ctx);
	}
}

BENCHMARK_DEFINE_F(SurfaceExtractorBenchmark, GreedyTextureSparse)(benchmark::State &state) {
	palette::Palette pal;
	pal.nippon();
	for (auto _ : state) {
		voxel::ChunkMesh mesh{65536, 65536, false};
		voxel::SurfaceExtractionContext ctx = voxel::buildGreedyTextureContext(&v, v.region(), mesh, pal);
		voxel::extractSurface(ctx);
	}
}

BENCHMARK_DEFINE_F(SurfaceExtractorBenchmark, CubicSolid)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::ChunkMesh mesh{65536, 65536, false};
		voxel::SurfaceExtractionContext ctx =
			voxel::buildCubicContext(&solid, solid.region(), mesh, glm::ivec3(0), true, true, false);
		voxel::extractSurface(ctx);
	}
}

BENCHMARK_DEFINE_F(SurfaceExtractorBenchmark, CubicSolidAO)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::ChunkMesh mesh{65536, 65536, false};
		voxel::SurfaceExtractionContext ctx =
			voxel::buildCubicContext(&solid, solid.region(), mesh, glm::ivec3(0), true, true, true);
		voxel::extractSurface(ctx);
	}
}

BENCHMARK_DEFINE_F(SurfaceExtractorBenchmark, BinarySolid)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::ChunkMesh mesh{65536, 65536, false};
		voxel::SurfaceExtractionContext ctx =
			voxel::buildBinaryContext(&solid, solid.region(), mesh, glm::ivec3(0), false);
		voxel::extractSurface(ctx);
	}
}

BENCHMARK_DEFINE_F(SurfaceExtractorBenchmark, BinarySolidAO)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::ChunkMesh mesh{65536, 65536, false};
		voxel::SurfaceExtractionContext ctx =
			voxel::buildBinaryContext(&solid, solid.region(), mesh, glm::ivec3(0), true);
		voxel::extractSurface(ctx);
	}
}

BENCHMARK_DEFINE_F(SurfaceExtractorBenchmark, MarchingCubesSolid)(benchmark::State &state) {
	palette::Palette pal;
	pal.nippon();
	for (auto _ : state) {
		voxel::ChunkMesh mesh{65536, 65536, false};
		voxel::SurfaceExtractionContext ctx = voxel::buildMarchingCubesContext(&solid, solid.region(), mesh, pal);
		voxel::extractSurface(ctx);
	}
}

BENCHMARK_DEFINE_F(SurfaceExtractorBenchmark, GreedyTextureSolid)(benchmark::State &state) {
	palette::Palette pal;
	pal.nippon();
	for (auto _ : state) {
		voxel::ChunkMesh mesh{65536, 65536, false};
		voxel::SurfaceExtractionContext ctx = voxel::buildGreedyTextureContext(&solid, solid.region(), mesh, pal);
		voxel::extractSurface(ctx);
	}
}

BENCHMARK_DEFINE_F(SurfaceExtractorBenchmark, CubicChecker)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::ChunkMesh mesh{65536, 65536, false};
		voxel::SurfaceExtractionContext ctx =
			voxel::buildCubicContext(&checker, checker.region(), mesh, glm::ivec3(0), true, true, false);
		voxel::extractSurface(ctx);
	}
}

BENCHMARK_DEFINE_F(SurfaceExtractorBenchmark, BinaryChecker)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::ChunkMesh mesh{65536, 65536, false};
		voxel::SurfaceExtractionContext ctx =
			voxel::buildBinaryContext(&checker, checker.region(), mesh, glm::ivec3(0), false);
		voxel::extractSurface(ctx);
	}
}

BENCHMARK_DEFINE_F(SurfaceExtractorBenchmark, MarchingCubesChecker)(benchmark::State &state) {
	palette::Palette pal;
	pal.nippon();
	for (auto _ : state) {
		voxel::ChunkMesh mesh{65536, 65536, false};
		voxel::SurfaceExtractionContext ctx = voxel::buildMarchingCubesContext(&checker, checker.region(), mesh, pal);
		voxel::extractSurface(ctx);
	}
}

BENCHMARK_DEFINE_F(SurfaceExtractorBenchmark, GreedyTextureChecker)(benchmark::State &state) {
	palette::Palette pal;
	pal.nippon();
	for (auto _ : state) {
		voxel::ChunkMesh mesh{65536, 65536, false};
		voxel::SurfaceExtractionContext ctx = voxel::buildGreedyTextureContext(&checker, checker.region(), mesh, pal);
		voxel::extractSurface(ctx);
	}
}

BENCHMARK_REGISTER_F(SurfaceExtractorBenchmark, CubicSparse);
BENCHMARK_REGISTER_F(SurfaceExtractorBenchmark, BinarySparse);
BENCHMARK_REGISTER_F(SurfaceExtractorBenchmark, MarchingCubesSparse);
BENCHMARK_REGISTER_F(SurfaceExtractorBenchmark, GreedyTextureSparse);
BENCHMARK_REGISTER_F(SurfaceExtractorBenchmark, CubicSolid);
BENCHMARK_REGISTER_F(SurfaceExtractorBenchmark, CubicSolidAO);
BENCHMARK_REGISTER_F(SurfaceExtractorBenchmark, BinarySolid);
BENCHMARK_REGISTER_F(SurfaceExtractorBenchmark, BinarySolidAO);
BENCHMARK_REGISTER_F(SurfaceExtractorBenchmark, MarchingCubesSolid);
BENCHMARK_REGISTER_F(SurfaceExtractorBenchmark, GreedyTextureSolid);
BENCHMARK_REGISTER_F(SurfaceExtractorBenchmark, CubicChecker);
BENCHMARK_REGISTER_F(SurfaceExtractorBenchmark, BinaryChecker);
BENCHMARK_REGISTER_F(SurfaceExtractorBenchmark, MarchingCubesChecker);
BENCHMARK_REGISTER_F(SurfaceExtractorBenchmark, GreedyTextureChecker);

BENCHMARK_MAIN();
