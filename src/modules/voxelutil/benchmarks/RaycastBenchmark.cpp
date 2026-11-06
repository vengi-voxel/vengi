/**
 * @file
 *
 * Benchmarks for cursor-style DDA raycasting. Distant-camera cases stress the
 * volume-AABB clip path (issue #588).
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/Raycast.h"

class RaycastBenchmark : public app::AbstractBenchmark {};

BENCHMARK_DEFINE_F(RaycastBenchmark, DistantHit)(benchmark::State &state) {
	voxel::RawVolume volume(voxel::Region{0, 63});
	volume.setVoxel(32, 32, 32, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	const glm::vec3 start(-5000.0f, 32.5f, 32.5f);
	const glm::vec3 end(5000.0f, 32.5f, 32.5f);
	auto stopOnSolid = [](voxel::RawVolume::Sampler &sampler) {
		return voxel::isAir(sampler.voxel().getMaterial());
	};
	for (auto _ : state) {
		auto result = voxelutil::raycastWithEndpoints(&volume, start, end, stopOnSolid);
		benchmark::DoNotOptimize(result);
	}
}

BENCHMARK_DEFINE_F(RaycastBenchmark, DistantMiss)(benchmark::State &state) {
	voxel::RawVolume volume(voxel::Region{0, 63});
	const glm::vec3 start(-5000.0f, 32.5f, 32.5f);
	const glm::vec3 end(5000.0f, 32.5f, 32.5f);
	auto alwaysContinue = [](voxel::RawVolume::Sampler &) { return true; };
	for (auto _ : state) {
		auto result = voxelutil::raycastWithEndpoints(&volume, start, end, alwaysContinue);
		benchmark::DoNotOptimize(result);
	}
}

BENCHMARK_DEFINE_F(RaycastBenchmark, DistantAABBMiss)(benchmark::State &state) {
	voxel::RawVolume volume(voxel::Region{0, 63});
	const glm::vec3 start(-5000.0f, 200.5f, 32.5f);
	const glm::vec3 end(5000.0f, 200.5f, 32.5f);
	auto alwaysContinue = [](voxel::RawVolume::Sampler &) { return true; };
	for (auto _ : state) {
		auto result = voxelutil::raycastWithEndpoints(&volume, start, end, alwaysContinue);
		benchmark::DoNotOptimize(result);
	}
}

BENCHMARK_DEFINE_F(RaycastBenchmark, InsideHit)(benchmark::State &state) {
	voxel::RawVolume volume(voxel::Region{0, 63});
	volume.setVoxel(40, 32, 32, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	const glm::vec3 start(0.5f, 32.5f, 32.5f);
	const glm::vec3 end(63.5f, 32.5f, 32.5f);
	auto stopOnSolid = [](voxel::RawVolume::Sampler &sampler) {
		return voxel::isAir(sampler.voxel().getMaterial());
	};
	for (auto _ : state) {
		auto result = voxelutil::raycastWithEndpoints(&volume, start, end, stopOnSolid);
		benchmark::DoNotOptimize(result);
	}
}

BENCHMARK_DEFINE_F(RaycastBenchmark, CursorFarPlaneMiss)(benchmark::State &state) {
	// Mirrors SceneManager cursor rays: camera far outside an empty volume with a long far plane.
	voxel::RawVolume volume(voxel::Region{0, 127});
	const glm::vec3 origin(64.0f, 64.0f, -4000.0f);
	const glm::vec3 direction = glm::vec3(0.0f, 0.0f, 8000.0f);
	auto alwaysContinue = [](voxel::RawVolume::Sampler &) { return true; };
	for (auto _ : state) {
		auto result = voxelutil::raycastWithDirection(&volume, origin, direction, alwaysContinue);
		benchmark::DoNotOptimize(result);
	}
}

BENCHMARK_REGISTER_F(RaycastBenchmark, DistantHit);
BENCHMARK_REGISTER_F(RaycastBenchmark, DistantMiss);
BENCHMARK_REGISTER_F(RaycastBenchmark, DistantAABBMiss);
BENCHMARK_REGISTER_F(RaycastBenchmark, InsideHit);
BENCHMARK_REGISTER_F(RaycastBenchmark, CursorFarPlaneMiss);
