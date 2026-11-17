/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "core/collection/Vector.h"
#include "voxel/RawVolume.h"
#include "voxel/SparseVolume.h"

class SparseVolumeBenchmark : public app::AbstractBenchmark {
protected:
	voxel::SparseVolume v{voxel::Region{0, 0, 0, 143, 22, 134}};
	voxel::SparseVolume vunlimit{};
};

BENCHMARK_DEFINE_F(SparseVolumeBenchmark, SetVoxel)(benchmark::State &state) {
	for (auto _ : state) {
		v.setVoxel(96, 6, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 7, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
}

BENCHMARK_DEFINE_F(SparseVolumeBenchmark, CalculateRegion)(benchmark::State &state) {
	for (auto _ : state) {
		for (int i = 0; i < 10; ++i)
			v.setVoxel(96, i, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		benchmark::DoNotOptimize(v.calculateRegion());
	}
}

BENCHMARK_DEFINE_F(SparseVolumeBenchmark, SetVoxelSampler)(benchmark::State &state) {
	voxel::SparseVolume::Sampler sampler(&v);
	for (auto _ : state) {
		sampler.setPosition(96, 6, 62);
		sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
		sampler.movePositiveY();
		sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
}

BENCHMARK_DEFINE_F(SparseVolumeBenchmark, SetVoxel_unlimit)(benchmark::State &state) {
	for (auto _ : state) {
		vunlimit.setVoxel(96, 6, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		vunlimit.setVoxel(96, 7, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
}

BENCHMARK_DEFINE_F(SparseVolumeBenchmark, SetVoxelSampler_unlimit)(benchmark::State &state) {
	voxel::SparseVolume::Sampler sampler(&vunlimit);
	for (auto _ : state) {
		sampler.setPosition(96, 6, 62);
		sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
		sampler.movePositiveY();
		sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
}

BENCHMARK_DEFINE_F(SparseVolumeBenchmark, SetVoxelsY)(benchmark::State &state) {
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	core::Vector<voxel::Voxel, 22> voxels;
	voxels.assign(voxel, voxels.capacity());
	for (auto _ : state) {
		voxel::setVoxels(v, 0, 0, &voxels.front(), v.region().getHeightInVoxels());
	}
}

BENCHMARK_DEFINE_F(SparseVolumeBenchmark, SetVoxels)(benchmark::State &state) {
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	core::Vector<voxel::Voxel, 22> voxels;
	voxels.assign(voxel, voxels.capacity());

	for (auto _ : state) {
		voxel::setVoxels(v, 0, 0, 0, v.region().getWidthInVoxels(), v.region().getDepthInVoxels(), &voxels.front(),
				  v.region().getHeightInVoxels());
	}
}

static void fillDense(voxel::SparseVolume &v, int size) {
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	for (int z = 0; z < size; ++z) {
		for (int y = 0; y < size; ++y) {
			for (int x = 0; x < size; ++x) {
				v.setVoxel(x, y, z, voxel);
			}
		}
	}
}

BENCHMARK_DEFINE_F(SparseVolumeBenchmark, FillDense)(benchmark::State &state) {
	const int size = (int)state.range(0);
	for (auto _ : state) {
		voxel::SparseVolume dense;
		fillDense(dense, size);
		benchmark::DoNotOptimize(dense.size());
	}
	state.SetItemsProcessed(state.iterations() * size * size * size);
}

BENCHMARK_DEFINE_F(SparseVolumeBenchmark, ReadDense)(benchmark::State &state) {
	const int size = (int)state.range(0);
	voxel::SparseVolume dense;
	fillDense(dense, size);
	for (auto _ : state) {
		int solid = 0;
		for (int z = 0; z < size; ++z) {
			for (int y = 0; y < size; ++y) {
				for (int x = 0; x < size; ++x) {
					solid += voxel::isBlocked(dense.voxel(x, y, z).getMaterial());
				}
			}
		}
		benchmark::DoNotOptimize(solid);
	}
	state.SetItemsProcessed(state.iterations() * size * size * size);
}

BENCHMARK_DEFINE_F(SparseVolumeBenchmark, CopyToRawVolume)(benchmark::State &state) {
	const int size = (int)state.range(0);
	voxel::SparseVolume dense;
	fillDense(dense, size);
	voxel::RawVolume target(voxel::Region(0, size - 1));
	for (auto _ : state) {
		dense.copyTo(target);
		benchmark::DoNotOptimize(target.data());
	}
	state.SetItemsProcessed(state.iterations() * size * size * size);
}

BENCHMARK_DEFINE_F(SparseVolumeBenchmark, CalculateRegionDense)(benchmark::State &state) {
	const int size = (int)state.range(0);
	voxel::SparseVolume dense;
	fillDense(dense, size);
	for (auto _ : state) {
		benchmark::DoNotOptimize(dense.calculateRegion());
	}
	state.SetItemsProcessed(state.iterations() * size * size * size);
}

BENCHMARK_DEFINE_F(SparseVolumeBenchmark, FillScattered)(benchmark::State &state) {
	const int count = (int)state.range(0);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	for (auto _ : state) {
		voxel::SparseVolume scattered;
		uint32_t seed = 1u;
		for (int i = 0; i < count; ++i) {
			seed = seed * 1664525u + 1013904223u;
			const int x = (int)(seed & 1023u) - 512;
			const int y = (int)((seed >> 10) & 1023u) - 512;
			const int z = (int)((seed >> 20) & 1023u) - 512;
			scattered.setVoxel(x, y, z, voxel);
		}
		benchmark::DoNotOptimize(scattered.size());
	}
	state.SetItemsProcessed(state.iterations() * count);
}

BENCHMARK_REGISTER_F(SparseVolumeBenchmark, SetVoxel);
BENCHMARK_REGISTER_F(SparseVolumeBenchmark, SetVoxelSampler);
BENCHMARK_REGISTER_F(SparseVolumeBenchmark, SetVoxel_unlimit);
BENCHMARK_REGISTER_F(SparseVolumeBenchmark, SetVoxelSampler_unlimit);
BENCHMARK_REGISTER_F(SparseVolumeBenchmark, CalculateRegion);
BENCHMARK_REGISTER_F(SparseVolumeBenchmark, SetVoxelsY);
BENCHMARK_REGISTER_F(SparseVolumeBenchmark, SetVoxels);
BENCHMARK_REGISTER_F(SparseVolumeBenchmark, FillDense)->Arg(64)->Arg(128)->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(SparseVolumeBenchmark, ReadDense)->Arg(64)->Arg(128)->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(SparseVolumeBenchmark, CopyToRawVolume)->Arg(128)->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(SparseVolumeBenchmark, CalculateRegionDense)->Arg(128)->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(SparseVolumeBenchmark, FillScattered)->Arg(10000)->Arg(100000)->Unit(benchmark::kMillisecond);

enum class ChunkRead { LinearScan, CoordinateScan, BrickRowScan, LinearCopy, RegionCopy };

static void sparseChunkRead(benchmark::State &state, ChunkRead read) {
	constexpr int Side = 128;
	constexpr int Cells = Side * Side * Side;
	// 0: dense; 1: one in eight bricks filled; 2: one plane per brick; 3: planes with holes.
	const int layout = (int)state.range(0);
	voxel::SparseChunk chunk;
	voxel::RawVolume linear(voxel::Region(0, Side - 1));
	voxel::RawVolume output(voxel::Region(0, Side - 1));
	for (int z = 0; z < Side; ++z) {
		for (int y = 0; y < Side; ++y) {
			for (int x = 0; x < Side; ++x) {
				constexpr int Shift = voxel::SparseChunk::BrickShift;
				constexpr int Mask = voxel::SparseChunk::BrickSide - 1;
				if ((layout == 1 && (((x >> Shift) + (y >> Shift) + (z >> Shift)) & 7) != 0) ||
					(layout >= 2 && (z & Mask) != 0) || (layout == 3 && ((x ^ y) & 3) == 0)) {
					continue;
				}
				const voxel::Voxel value = voxel::createVoxel(voxel::VoxelType::Generic, (uint8_t)(1 + (x ^ y ^ z)));
				chunk.setVoxel(x, y, z, value);
				linear.voxels()[x + Side * (y + Side * z)].overwrite(value);
			}
		}
	}
	for (auto _ : state) {
		int sum = 0;
		switch (read) {
		case ChunkRead::LinearScan:
			for (int i = 0; i < Cells; ++i) {
				sum += linear.voxels()[i].getColor();
			}
			break;
		case ChunkRead::CoordinateScan:
			for (int z = 0; z < Side; ++z) {
				for (int y = 0; y < Side; ++y) {
					for (int x = 0; x < Side; ++x) {
						const voxel::Voxel *v = chunk.voxel(x, y, z);
						if (v != nullptr) {
							sum += v->getColor();
						}
					}
				}
			}
			break;
		case ChunkRead::BrickRowScan: {
			constexpr int BrickSide = voxel::SparseChunk::BrickSide;
			voxel::Voxel scratch[BrickSide];
			for (int z = 0; z < Side; ++z) {
				for (int y = 0; y < Side; ++y) {
					for (int x = 0; x < Side; x += BrickSide) {
						const voxel::Voxel *row = chunk.readBrickRow(x, y, z, scratch);
						for (int i = 0; i < BrickSide; ++i) {
							sum += row[i].getColor();
						}
					}
				}
			}
			break;
		}
		case ChunkRead::LinearCopy:
			memcpy((void *)output.voxels(), linear.data(), Cells * sizeof(voxel::Voxel));
			break;
		case ChunkRead::RegionCopy:
			chunk.readRegion(glm::ivec3(0), glm::ivec3(Side), output.voxels());
			break;
		}
		benchmark::DoNotOptimize(sum);
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations() * Cells);
}

BENCHMARK_CAPTURE(sparseChunkRead, LinearScan, ChunkRead::LinearScan)->DenseRange(0, 3)->ArgName("layout")->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(sparseChunkRead, CoordinateScan, ChunkRead::CoordinateScan)->DenseRange(0, 3)->ArgName("layout")->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(sparseChunkRead, BrickRowScan, ChunkRead::BrickRowScan)->DenseRange(0, 3)->ArgName("layout")->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(sparseChunkRead, LinearCopy, ChunkRead::LinearCopy)->DenseRange(0, 3)->ArgName("layout")->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(sparseChunkRead, RegionCopy, ChunkRead::RegionCopy)->DenseRange(0, 3)->ArgName("layout")->Unit(benchmark::kMillisecond);
