/**
 * @file
 */

#pragma once

#include "voxel/polyvox/Mesh.h"
#include "voxel/polyvox/RawVolume.h"
#include "math/Random.h"
#include "voxel/RandomVoxel.h"
#include "core/GLM.h"
#include "PlantType.h"

namespace voxel {

class PlantGenerator {
private:
	Mesh* _meshes[(int)PlantType::MaxPlantTypes];
	math::Random _random;

public:
	PlantGenerator();
	~PlantGenerator();

	template<class Volume>
	void createFlower(int size, glm::ivec3 pos, Volume& volume) const {
		const RandomVoxel stalk(VoxelType::Grass, _random);
		for (int i = 0; i < size - 2; ++i) {
			volume.setVoxel(pos, stalk);
			++pos.y;
		}
		volume.setVoxel(pos, RandomVoxel(VoxelType::Bloom, _random));
		const RandomVoxel voxel(VoxelType::Flower, _random);
		--pos.x;
		volume.setVoxel(pos, stalk);
		--pos.z;
		++pos.x;
		volume.setVoxel(pos, stalk);
		pos.z += 2;
		volume.setVoxel(pos, stalk);
		--pos.z;
		++pos.x;
		volume.setVoxel(pos, stalk);
	}

	template<class Volume>
	void createGrass(int size, const glm::ivec3& pos, Volume& volume) const {
		// TODO: use noise
		const RandomVoxel stalk(VoxelType::Grass, _random);
		glm::ivec3 p = pos;
		for (int i = 0; i < size; ++i) {
			volume.setVoxel(p, stalk);
			++p.y;
		}
		p.y = pos.y;
		p.x -= size / 2 - 1;
		for (int i = 0; i < size - 2; ++i) {
			volume.setVoxel(p, stalk);
			++p.y;
		}

		p.y = pos.y;
		p.x = pos.x;
		p.x += size / 2 - 1;
		for (int i = 0; i < size - 2; ++i) {
			volume.setVoxel(p, stalk);
			++p.y;
		}
	}

	template<class Volume>
	void createMushroom(int size, glm::ivec3 pos, Volume& volume) const {
		const RandomVoxel voxel(VoxelType::Mushroom, _random);
		for (int i = 0; i < 3; ++i) {
			volume.setVoxel(pos, voxel);
			++pos.y;
		}
		const int height = size - 3;
		const double minRadius = size / 2.0;
		const double heightFactor = height / (height - 1.0) / 2.0;
		for (int y = 0; y <= height; ++y) {
			const double percent = y / heightFactor;
			const double circleRadius = glm::pow(minRadius, 2.0) - glm::pow(percent, 2.0);
			const glm::ivec3 planePos(pos.x, pos.y + y, pos.z);
			const int radius = height / 2;
			const double ratio = radius / minRadius;

			for (int z = -radius; z <= radius; ++z) {
				for (int x = -radius; x <= radius; ++x) {
					const double distance = glm::pow(x / ratio, 2.0) + glm::pow(z / ratio, 2.0);
					if (distance > circleRadius) {
						continue;
					}
					volume.setVoxel({planePos.x + x, planePos.y, planePos.z + z}, voxel);
				}
			}
		}
	}

	void shutdown();

	void generateAll();
	bool generatePlant(int size, PlantType type, Mesh *result);
	Mesh* getMesh(PlantType type);
};

}
