/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "core/collection/Vector.h"
#include "voxel/ConcurrentSparseVolume.h"

class ConcurrentSparseVolumeBenchmark : public app::AbstractBenchmark {
protected:
	voxel::ConcurrentSparseVolume v{voxel::Region{0, 0, 0, 143, 22, 134}};
	voxel::ConcurrentSparseVolume vunlimit{};
};

BENCHMARK_DEFINE_F(ConcurrentSparseVolumeBenchmark, SetVoxel)(benchmark::State &state) {
	for (auto _ : state) {
		v.setVoxel(96, 6, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 7, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
}

BENCHMARK_DEFINE_F(ConcurrentSparseVolumeBenchmark, CalculateRegion)(benchmark::State &state) {
	for (auto _ : state) {
		for (int i = 0; i < 10; ++i)
			v.setVoxel(96, i, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		benchmark::DoNotOptimize(v.calculateRegion());
	}
}

BENCHMARK_DEFINE_F(ConcurrentSparseVolumeBenchmark, SetVoxelSampler)(benchmark::State &state) {
	voxel::ConcurrentSparseVolume::Sampler sampler(&v);
	for (auto _ : state) {
		sampler.setPosition(96, 6, 62);
		sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
		sampler.movePositiveY();
		sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
}

BENCHMARK_DEFINE_F(ConcurrentSparseVolumeBenchmark, SetVoxel_unlimit)(benchmark::State &state) {
	for (auto _ : state) {
		vunlimit.setVoxel(96, 6, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		vunlimit.setVoxel(96, 7, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
}

BENCHMARK_DEFINE_F(ConcurrentSparseVolumeBenchmark, SetVoxelSampler_unlimit)(benchmark::State &state) {
	voxel::ConcurrentSparseVolume::Sampler sampler(&vunlimit);
	for (auto _ : state) {
		sampler.setPosition(96, 6, 62);
		sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
		sampler.movePositiveY();
		sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
}

BENCHMARK_DEFINE_F(ConcurrentSparseVolumeBenchmark, SetVoxelsY)(benchmark::State &state) {
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	core::Vector<voxel::Voxel, 22> voxels;
	voxels.assign(voxel, voxels.capacity());
	for (auto _ : state) {
		voxel::setVoxels(v, 0, 0, &voxels.front(), v.region().getHeightInVoxels());
	}
}

BENCHMARK_DEFINE_F(ConcurrentSparseVolumeBenchmark, SetVoxels)(benchmark::State &state) {
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	core::Vector<voxel::Voxel, 22> voxels;
	voxels.assign(voxel, voxels.capacity());

	for (auto _ : state) {
		voxel::setVoxels(v, 0, 0, 0, v.region().getWidthInVoxels(), v.region().getDepthInVoxels(), &voxels.front(),
				  v.region().getHeightInVoxels());
	}
}

BENCHMARK_DEFINE_F(ConcurrentSparseVolumeBenchmark, FillDenseParallel)(benchmark::State &state) {
	const int size = (int)state.range(0);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	for (auto _ : state) {
		voxel::ConcurrentSparseVolume dense;
		app::for_parallel(0, size, [&dense, &voxel, size](int start, int end) {
			for (int z = start; z < end; ++z) {
				for (int y = 0; y < size; ++y) {
					for (int x = 0; x < size; ++x) {
						dense.setVoxel(x, y, z, voxel);
					}
				}
			}
		});
		benchmark::DoNotOptimize(dense.size());
	}
	state.SetItemsProcessed(state.iterations() * size * size * size);
}

BENCHMARK_DEFINE_F(ConcurrentSparseVolumeBenchmark, ReadDenseParallel)(benchmark::State &state) {
	const int size = (int)state.range(0);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	voxel::ConcurrentSparseVolume dense;
	for (int z = 0; z < size; ++z) {
		dense.setVoxelsRow(0, 0, z, size, voxel);
		for (int y = 1; y < size; ++y) {
			dense.setVoxelsRow(0, y, z, size, voxel);
		}
	}
	for (auto _ : state) {
		core::AtomicInt solid{0};
		app::for_parallel(0, size, [&dense, &solid, size](int start, int end) {
			int localSolid = 0;
			for (int z = start; z < end; ++z) {
				for (int y = 0; y < size; ++y) {
					for (int x = 0; x < size; ++x) {
						localSolid += voxel::isBlocked(dense.voxel(x, y, z).getMaterial());
					}
				}
			}
			solid.increment(localSolid);
		});
		benchmark::DoNotOptimize((int)solid);
	}
	state.SetItemsProcessed(state.iterations() * size * size * size);
}

BENCHMARK_REGISTER_F(ConcurrentSparseVolumeBenchmark, SetVoxel);
BENCHMARK_REGISTER_F(ConcurrentSparseVolumeBenchmark, SetVoxelSampler);
BENCHMARK_REGISTER_F(ConcurrentSparseVolumeBenchmark, SetVoxel_unlimit);
BENCHMARK_REGISTER_F(ConcurrentSparseVolumeBenchmark, SetVoxelSampler_unlimit);
BENCHMARK_REGISTER_F(ConcurrentSparseVolumeBenchmark, CalculateRegion);
BENCHMARK_REGISTER_F(ConcurrentSparseVolumeBenchmark, SetVoxelsY);
BENCHMARK_REGISTER_F(ConcurrentSparseVolumeBenchmark, SetVoxels);
BENCHMARK_REGISTER_F(ConcurrentSparseVolumeBenchmark, FillDenseParallel)->Arg(128)->Unit(benchmark::kMillisecond)->UseRealTime();
BENCHMARK_REGISTER_F(ConcurrentSparseVolumeBenchmark, ReadDenseParallel)->Arg(128)->Unit(benchmark::kMillisecond)->UseRealTime();
