#include "PlantGenerator.h"
#include "voxel/polyvox/CubicSurfaceExtractor.h"
#include "voxel/IsQuadNeeded.h"

namespace voxel {

PlantGenerator::PlantGenerator() {
	for (int i = 0; i < MaxPlantTypes; ++i) {
		_meshes[i] = nullptr;
	}
}

PlantGenerator::~PlantGenerator() {
	shutdown();
}

void PlantGenerator::shutdown() {
	for (int i = 0; i < MaxPlantTypes; ++i) {
		delete _meshes[i];
		_meshes[i] = nullptr;
	}
}

void PlantGenerator::createFlower(int size, glm::ivec3 pos, RawVolume& volume) const {
	const Voxel stalk = createVoxel(Grass1);
	for (int i = 0; i < size - 2; ++i) {
		volume.setVoxel(pos, stalk);
		++pos.y;
	}
	volume.setVoxel(pos, createVoxel(Leaves10));
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

void PlantGenerator::createGrass(int size, glm::ivec3 pos, RawVolume& volume) const {
	// TODO: use noise
	const Voxel stalk = createVoxel(Grass1);
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

void PlantGenerator::createMushroom(int size, glm::ivec3 pos, RawVolume& volume) const {
	const Voxel stalk = createVoxel(Grass1);
	for (int i = 0; i < 3; ++i) {
		volume.setVoxel(pos, stalk);
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

		const Voxel voxel = createVoxel(Leaves10);
		for (int z = -radius; z <= radius; ++z) {
			for (int x = -radius; x <= radius; ++x) {
				const double distance = glm::pow(x / ratio, 2.0) + glm::pow(z / ratio, 2.0);
				if (distance > circleRadius) {
					continue;
				}
				const glm::ivec3 pos(planePos.x + x, planePos.y, planePos.z + z);
				volume.setVoxel(pos, voxel);
			}
		}
	}
}

bool PlantGenerator::generatePlant(int size, PlantType type, Mesh *result) {
	const Region region(0, 0, 0, size, size, size);
	RawVolume volume(region);
	volume.setBorderValue(createVoxel(Air));
	glm::ivec3 pos = region.getCentre();
	pos.y = 0;
	switch (type) {
	case Flower:
		createFlower(size, pos, volume);
		break;
	case Mushroom:
		createMushroom(size, pos, volume);
		break;
	case Grass:
		createGrass(size, pos, volume);
		break;
	default:
		return false;
	}

	extractCubicMesh(&volume, region, result, IsQuadNeeded(false));

	return true;
}

Mesh* PlantGenerator::getMesh(PlantType type) {
	return _meshes[type];
}

void PlantGenerator::generateAll() {
	for (int t = 0; t < MaxPlantTypes; ++t) {
		_meshes[t] = new Mesh(1000, 1000);
		int size;
		switch (t) {
		case Grass:
			size = 10;
			break;
		case Mushroom:
			size = 7;
			break;
		default:
			size = 5;
			break;
		}
		generatePlant(size, (PlantType)t, _meshes[t]);
	}
}

}
