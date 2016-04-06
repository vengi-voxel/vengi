#include "World.h"
#include "core/App.h"
#include "core/ByteStream.h"
#include "core/Var.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/Trace.h"
#include "io/File.h"
#include "Raycast.h"
#include "Voxel.h"
#include "core/Random.h"
#include "noise/SimplexNoise.h"
#include <PolyVox/AStarPathfinder.h>
#include <PolyVox/CubicSurfaceExtractor.h>
#include <PolyVox/MarchingCubesSurfaceExtractor.h>
#include <PolyVox/AmbientOcclusionCalculator.h>
#include <PolyVox/RawVolume.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL.h>
#include <zlib.h>

namespace voxel {

#define WORLD_FILE_VERSION 1

void World::Pager::pageIn(const PolyVox::Region& region, WorldData::Chunk* chunk) {
	if (!_world.load(region, chunk)) {
		_world.create(region, chunk);
	}
}

void World::Pager::pageOut(const PolyVox::Region& region, WorldData::Chunk* chunk) {
	//_world.save(region, chunk);
}

// http://code.google.com/p/fortressoverseer/source/browse/Overseer/PolyVoxGenerator.cpp
World::World() :
		_pager(*this), _seed(0), _clientData(false), _threadPool(1), _rwLock("World"),
		_random(_seed), _noiseSeedOffsetX(0.0f), _noiseSeedOffsetZ(0.0f) {
	_chunkSize = core::Var::get(cfg::VoxelChunkSize, "64", core::CV_READONLY);
	_volumeData = new WorldData(&_pager, 256 * 1024 * 1024, _chunkSize->intVal());
	core_assert(_biomManager.addBiom(0, 100, Voxel(Grass, Voxel::getMaxDensity())));
	core_assert(_biomManager.addBiom(101, MAX_HEIGHT - 1, Voxel(Grass, Voxel::getMaxDensity())));
}

World::~World() {
}

int World::findChunkFloor(int chunkHeight, WorldData::Chunk* chunk, int x, int z) {
	for (int i = chunkHeight - 1; i >= 0; i--) {
		const int material = chunk->getVoxel(x, i, z).getMaterial();
		if (material != Air && material != Cloud) {
			return i + 1;
		}
	}
	return -1;
}

glm::ivec2 World::randomPosWithoutHeight(const PolyVox::Region& region, core::Random& random, int border) {
	const int w = region.getWidthInVoxels();
	const int d = region.getDepthInVoxels();
	core_assert(border < w);
	core_assert(border < d);
	const int x = random.random(border, w - border);
	const int z = random.random(border, d - border);
	return glm::ivec2(x, z);
}

glm::ivec3 World::randomPos() const {
	const glm::ivec2 pos(0, 0);
	const int y = findFloor(pos.x, pos.y);
	return glm::ivec3(pos.x, y, pos.y);
}

struct IsVoxelTransparent {
	bool operator()(const Voxel& voxel) const {
		return voxel.getMaterial() == Air;
	}
};

struct IsQuadNeeded {
	bool operator()(Voxel back, Voxel front, Voxel& materialToUse) {
		if (back.getMaterial() != Air && front.getMaterial() == Air) {
			materialToUse = back;
			return true;
		} else {
			return false;
		}
	}
};

void World::calculateAO(const PolyVox::Region& region) {
	for (int nx = region.getLowerX() - 1; nx < region.getUpperX() + 1; ++nx) {
		for (int nz = region.getLowerZ() - 1; nz < region.getUpperZ() + 1; ++nz) {
			for (int ny = region.getLowerY(); ny < region.getUpperY() - 1; ++ny) {
				// if the voxel is air, we don't need to compute anything
				Voxel voxel = _volumeData->getVoxel(nx, ny, nz);
				if (voxel.getMaterial() == Air) {
					continue;
				}
				// if the voxel above us is not free - we don't calculate ao for this voxel
				if (_volumeData->getVoxel(nx, ny + 1, nz).getMaterial() != Air) {
					continue;
				}
				static const struct offsets {
					int x;
					int z;
				} of[] = {
					{ 1,  0}, { 1, -1}, {0, -1}, {-1, -1},
					{-1,  0}, {-1,  1}, {0,  1}, { 1,  1}
				};
				// reduce ao value to make a voxel face darker
				uint8_t ao = 255;
				for (int i = 0; i < (int)SDL_arraysize(of); ++i) {
					const int offX = of[i].x;
					const int offZ = of[i].z;
					const Voxel& voxel = _volumeData->getVoxel(nx + offX, ny + 1, nz + offZ);
					if (voxel.getMaterial() != Air) {
						ao -= 25;
					}
				}
				voxel.setDensity(ao);
				_volumeData->setVoxel(nx, ny, nz, voxel);
			}
		}
	}
}

// Extract the surface for the specified region of the volume.
// The surface extractor outputs the mesh in an efficient compressed format which
// is not directly suitable for rendering.
bool World::scheduleMeshExtraction(const glm::ivec2& p) {
	const glm::ivec2& pos = getGridPos(p);
	auto i = _meshesExtracted.find(pos);
	if (i != _meshesExtracted.end()) {
		Log::trace("mesh is already extracted for %i:%i (%i:%i - %i:%i)", p.x, p.y, i->x, i->y, pos.x, pos.y);
		return false;
	}
	Log::trace("mesh extraction for %i:%i (%i:%i)", p.x, p.y, pos.x, pos.y);
	_meshesExtracted.insert(pos);

	const int size = _chunkSize->intVal();
	const int delta = size - 1;
	_threadPool.enqueue([=] () {
		core_trace_scoped("MeshExtraction");
		const PolyVox::Vector3DInt32 mins(pos.x, 0, pos.y);
		const PolyVox::Vector3DInt32 maxs(mins.getX() + delta, MAX_HEIGHT - 1, mins.getZ() + delta);
		const PolyVox::Region region(mins, maxs);
		DecodedMeshData data;
		{
			locked([&] () {
				// calculate ao
				calculateAO(region);
#if 0
				data.mesh = PolyVox::decodeMesh(PolyVox::extractMarchingCubesMesh(_volumeData, region));
#else
				data.mesh = PolyVox::decodeMesh(PolyVox::extractCubicMesh(_volumeData, region, IsQuadNeeded(), true));
#endif
			});
		}

		const PolyVox::Vector3DInt32& offset = data.mesh.getOffset();
		data.translation = glm::ivec2(offset.getX(), offset.getZ());
		core::ScopedWriteLock lock(_rwLock);
		_meshQueue.push_back(std::move(data));
	});
	return true;
}

int World::findFloor(int x, int z) const {
	for (int i = MAX_HEIGHT; i >= 0; i--) {
		const int material = getMaterial(x, i, z);
		if (material != Air && material != Cloud) {
			return i + 1;
		}
	}
	return -1;
}

void World::allowReExtraction(const glm::ivec2& pos) {
	_meshesExtracted.erase(getGridPos(pos));
}

World::Result World::raycast(const glm::vec3& start, const glm::vec3& end, voxel::Raycast& raycast) {
	return locked([&] () {
		PolyVox::RaycastResult result = PolyVox::raycastWithEndpoints(_volumeData, PolyVox::Vector3DFloat(start.x, start.y, start.z),
				PolyVox::Vector3DFloat(end.x, end.y, end.z), raycast);
		if (result == PolyVox::RaycastResults::Completed)
			return World::COMPLETED;
		return World::INTERUPTED;
	});
}

bool World::findPath(const PolyVox::Vector3DInt32& start, const PolyVox::Vector3DInt32& end,
		std::list<PolyVox::Vector3DInt32>& listResult) {
	static auto f = [] (const voxel::WorldData* volData, const PolyVox::Vector3DInt32& v3dPos) {
		voxel::Voxel voxel = volData->getVoxel(v3dPos);
		return voxel.getDensity() != 0;
	};

	locked([&] () {
		const PolyVox::AStarPathfinderParams<voxel::WorldData> params(_volumeData, start, end, &listResult, 1.0f, 10000,
				PolyVox::TwentySixConnected, std::bind(f, std::placeholders::_1, std::placeholders::_2));
		PolyVox::AStarPathfinder<voxel::WorldData> pf(params);
		// TODO: move into threadpool
		pf.execute();
	});
	return true;
}

void World::destroy() {
	locked([this] () {
		_volumeData->flushAll();
		_seed = 0l;
		Log::info("flush the world");
	});
}

void World::createCirclePlane(const PolyVox::Region& region, WorldData::Chunk* chunk, const glm::ivec3& center, int width, int depth, double radius, const Voxel& voxel) {
	const int xRadius = width / 2;
	const int zRadius = depth / 2;
	const double minRadius = std::min(xRadius, zRadius);
	const double ratioX = xRadius / minRadius;
	const double ratioZ = zRadius / minRadius;

	for (int z = -zRadius; z <= zRadius; ++z) {
		for (int x = -xRadius; x <= xRadius; ++x) {
			const double distance = glm::pow(x / ratioX, 2.0) + glm::pow(z / ratioZ, 2.0);
			if (distance > radius) {
				continue;
			}
			chunk->setVoxel(center.x + x, center.y, center.z + z, voxel);
		}
	}
}

void World::createCube(const PolyVox::Region& region, WorldData::Chunk* chunk, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel) {
	const int w = width / 2;
	const int h = height / 2;
	const int d = depth / 2;
	for (int x = -w; x < width - w; ++x) {
		for (int y = -h; y < height - h; ++y) {
			for (int z = -d; z < depth - d; ++z) {
				chunk->setVoxel(pos.x + x, pos.y + y, pos.z + z, voxel);
			}
		}
	}
}

void World::createPlane(const PolyVox::Region& region, WorldData::Chunk* chunk, const glm::ivec3& pos, int width, int depth, const Voxel& voxel) {
	createCube(region, chunk, pos, width, 1, depth, voxel);
}

void World::createEllipse(const PolyVox::Region& region, WorldData::Chunk* chunk, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel) {
	const int heightLow = height / 2;
	const int heightHigh = height - heightLow;
	const double minDimension = std::min(width, depth);
	const double adjustedMinRadius = minDimension / 2.0;
	const double heightFactor = heightLow / adjustedMinRadius;
	for (int y = -heightLow; y <= heightHigh; ++y) {
		const double percent = glm::abs(y / heightFactor);
		const double circleRadius = glm::pow(adjustedMinRadius + 0.5, 2.0) - glm::pow(percent, 2.0);
		const glm::ivec3 planePos(pos.x, pos.y + y, pos.z);
		createCirclePlane(region, chunk, planePos, width, depth, circleRadius, voxel);
	}
}

void World::createCone(const PolyVox::Region& region, WorldData::Chunk* chunk, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel) {
	const int heightLow = height / 2;
	const int heightHigh = height - heightLow;
	const double minDimension = std::min(width, depth);
	const double minRadius = minDimension / 2.0;
	for (int y = -heightLow; y <= heightHigh; ++y) {
		const double percent = 1.0 - ((y + heightLow) / double(height));
		const double circleRadius = glm::pow(percent * minRadius, 2.0);
		const glm::ivec3 planePos(pos.x, pos.y + y, pos.z);
		createCirclePlane(region, chunk, planePos, width, depth, circleRadius, voxel);
	}
}

void World::createDome(const PolyVox::Region& region, WorldData::Chunk* chunk, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel) {
	const int heightLow = height / 2;
	const int heightHigh = height - heightLow;
	const double minDimension = std::min(width, depth);
	const double minRadius = minDimension / 2.0;
	const double heightFactor = height / (minDimension - 1.0) / 2.0;
	for (int y = -heightLow; y <= heightHigh; ++y) {
		const double percent = glm::abs((y + heightLow) / heightFactor);
		const double circleRadius = glm::pow(minRadius, 2.0) - glm::pow(percent, 2.0);
		const glm::ivec3 planePos(pos.x, pos.y + y, pos.z);
		createCirclePlane(region, chunk, planePos, width, depth, circleRadius, voxel);
	}
}

void World::addTree(const PolyVox::Region& region, WorldData::Chunk* chunk, const glm::ivec3& pos, TreeType type, int trunkHeight, int trunkWidth, int width, int depth, int height) {
	const int top = (int) pos.y + trunkHeight;

	const int chunkHeight = region.getHeightInVoxels();

	const Voxel voxel(Wood, Voxel::getMaxDensity());
	for (int y = pos.y; y < top; ++y) {
		const int trunkWidthY = trunkWidth + std::max(0, 2 - (y - pos.y));
		for (int x = pos.x - trunkWidthY; x < pos.x + trunkWidthY; ++x) {
			for (int z = pos.z - trunkWidthY; z < pos.z + trunkWidthY; ++z) {
				if ((x >= pos.x + trunkWidthY || x < pos.x - trunkWidthY) && (z >= pos.z + trunkWidthY || z < pos.z - trunkWidthY)) {
					continue;
				}
				int y1 = y;
				if (y1 == pos.y) {
					y1 = findChunkFloor(chunkHeight, chunk, x, z);
					if (y1 < 0) {
						continue;
					}
				}
				chunk->setVoxel(x, y1, z, voxel);
			}
		}
	}

	// TODO: random leaves
	const Voxel leavesVoxel(Leaves1, 1);
	const glm::ivec3 leafesPos(pos.x, top + height / 2, pos.z);
	if (type == TreeType::ELLIPSIS) {
		createEllipse(region, chunk, leafesPos, width, height, depth, leavesVoxel);
	} else if (type == TreeType::CONE) {
		createCone(region, chunk, leafesPos, width, height, depth, leavesVoxel);
	} else if (type == TreeType::DOME) {
		createDome(region, chunk, leafesPos, width, height, depth, leavesVoxel);
	} else if (type == TreeType::CUBE) {
		createCube(region, chunk, leafesPos, width, height, depth, leavesVoxel);
		// TODO: use CreatePlane
		createCube(region, chunk, leafesPos, width + 2, height - 2, depth - 2, leavesVoxel);
		createCube(region, chunk, leafesPos, width - 2, height + 2, depth - 2, leavesVoxel);
		createCube(region, chunk, leafesPos, width - 2, height - 2, depth + 2, leavesVoxel);
	}
}

void World::createTrees(const PolyVox::Region& region, WorldData::Chunk* chunk, core::Random& random) {
	const int chunkHeight = region.getHeightInVoxels();
	for (int i = 0; i < 5; ++i) {
		const int maxSize = 14;
		const int rndValX = random.random(maxSize, region.getWidthInVoxels() - 1 - maxSize);
		// number should be even
		if (!(rndValX % 2)) {
			continue;
		}

		const int rndValZ = random.random(maxSize, region.getDepthInVoxels() - 1 - maxSize);
		// TODO: use a noise map to get the position
		glm::ivec3 pos(rndValX, -1, rndValZ);
		const int y = findChunkFloor(chunkHeight, chunk, pos.x, pos.z);
		const int height = random.random(10, 14);
		const int trunkHeight = random.random(5, 9);
		if (y < 0 || y >= chunkHeight - height - trunkHeight) {
			continue;
		}

		pos.y = y;

		const int size = random.random(12, maxSize);
		const int treeType = random.random(0, int(TreeType::MAX) - 1);
		const int trunkWidth = 1;
		addTree(region, chunk, pos, (TreeType)treeType, trunkHeight, trunkWidth, size, size, height);
	}
}

void World::createClouds(const PolyVox::Region& region, WorldData::Chunk* chunk, core::Random& random) {
	const int amount = 4;
	static const Voxel voxel(Cloud, Voxel::getMinDensity());
	for (int i = 0; i < amount; ++i) {
		const int height = 10;
		const glm::ivec2& pos = randomPosWithoutHeight(region, random, 20);
		glm::ivec3 chunkCloudCenterPos(pos.x, region.getHeightInVoxels() - height, pos.y);
		createEllipse(region, chunk, chunkCloudCenterPos, 10, height, 10, voxel);
		chunkCloudCenterPos.x -= 5;
		chunkCloudCenterPos.y -= 5 + i;
		createEllipse(region, chunk, chunkCloudCenterPos, 20, height, 20, voxel);
	}
}

void World::createUnderground(const PolyVox::Region& region, WorldData::Chunk* chunk) {
	glm::ivec3 startPos(1, 1, 1);
	const Voxel voxel(Grass, Voxel::getMaxDensity());
	createPlane(region, chunk, startPos, 10, 10, voxel);
}

bool World::load(const PolyVox::Region& region, WorldData::Chunk* chunk) {
	const core::App* app = core::App::getInstance();
	const io::FilesystemPtr& filesystem = app->filesystem();
	const std::string& filename = core::string::format("world-%li-%i-%i-%i.wld", _seed, region.getCentreX(), region.getCentreY(), region.getCentreZ());
	const io::FilePtr& f = filesystem->open(filename);
	if (!f->exists()) {
		return false;
	}
	Log::trace("Try to load world %s", f->getName().c_str());
	uint8_t *fileBuf;
	// TODO: load async, put world into state loading, and do the real loading in onFrame if the file is fully loaded
	const int fileLen = f->read((void **) &fileBuf);
	if (!fileBuf || fileLen <= 0) {
		Log::error("Failed to load the world from %s", f->getName().c_str());
		return false;
	}
	std::unique_ptr<uint8_t[]> smartBuf(fileBuf);

	core::ByteStream bs;
	bs.append(fileBuf, fileLen);
	int len;
	int version;
	bs.readFormat("ib", &len, &version);

	if (version != WORLD_FILE_VERSION) {
		Log::error("file %s has a wrong version number %i (expected %i)", f->getName().c_str(), version, WORLD_FILE_VERSION);
		return false;
	}
	const int sizeLimit = 1024;
	if (len > 1000l * 1000l * sizeLimit) {
		Log::error("extracted memory would be more than %i MB for the file %s", sizeLimit, f->getName().c_str());
		return false;
	}

	Log::info("Loading a world from file %s,uncompressing to %i", f->getName().c_str(), (int) len);

	uint8_t* targetBuf = new uint8_t[len];
	std::unique_ptr<uint8_t[]> smartTargetBuf(targetBuf);

	uLongf targetBufSize = len;
	const int res = uncompress(targetBuf, &targetBufSize, bs.getBuffer(), bs.getSize());
	if (res != Z_OK) {
		Log::error("Failed to uncompress the world data with len %i", len);
		return false;
	}

	core::ByteStream voxelBuf(len);
	voxelBuf.append(targetBuf, len);

	const int width = region.getWidthInVoxels();
	const int height = region.getHeightInVoxels();
	const int depth = region.getDepthInVoxels();

	for (int z = 0; z < depth; ++z) {
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				core_assert(voxelBuf.getSize() >= 2);
				const uint8_t material = voxelBuf.readByte();
				const uint8_t density = voxelBuf.readByte();
				const Voxel voxel(material, density);
				chunk->setVoxel(x, y, z, voxel);
			}
		}
	}
	return true;
}

bool World::save(const PolyVox::Region& region, WorldData::Chunk* chunk) {
	Log::info("Save chunk");
	core::ByteStream voxelStream;
	const int width = region.getWidthInVoxels();
	const int height = region.getHeightInVoxels();
	const int depth = region.getDepthInVoxels();

	for (int z = 0; z < depth; ++z) {
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				const Voxel& voxel = chunk->getVoxel(x, y, z);
				voxelStream.addByte(voxel.getMaterial());
				voxelStream.addByte(voxel.getDensity());
			}
		}
	}

	// save the stuff
	const std::string filename = core::string::format("world-%li-%i-%i-%i.wld", _seed, region.getCentreX(), region.getCentreY(), region.getCentreZ());
	const core::App* app = core::App::getInstance();
	const io::FilesystemPtr& filesystem = app->filesystem();

	const uint8_t* voxelBuf = voxelStream.getBuffer();
	const int voxelSize = voxelStream.getSize();
	uLongf neededVoxelBufLen = compressBound(voxelSize);
	uint8_t* compressedVoxelBuf = new uint8_t[neededVoxelBufLen];
	std::unique_ptr<uint8_t[]> smartBuf(compressedVoxelBuf);
	const int res = compress(compressedVoxelBuf, &neededVoxelBufLen, voxelBuf, voxelSize);
	if (res != Z_OK) {
		Log::error("Failed to compress the voxel data");
		return false;
	}
	core::ByteStream final;
	final.addFormat("ib", voxelSize, WORLD_FILE_VERSION);
	final.append(compressedVoxelBuf, neededVoxelBufLen);
	if (!filesystem->write(filename, final.getBuffer(), final.getSize())) {
		Log::error("Failed to write file %s", filename.c_str());
		return false;
	}
	Log::info("Wrote file %s (%i)", filename.c_str(), voxelSize);
	return true;
}

void World::create(const PolyVox::Region& region, WorldData::Chunk* chunk) {
	Log::debug("Create new chunk at %i:%i:%i", region.getCentreX(), region.getCentreY(), region.getCentreZ());
	const int width = region.getWidthInVoxels();
	const int depth = region.getDepthInVoxels();
	const int height = region.getHeightInVoxels();
	const int lowerY = region.getLowerY();
	const int lowerX = region.getLowerX();
	const int lowerZ = region.getLowerZ();
	for (int z = 0; z < depth; ++z) {
		for (int x = 0; x < width; ++x) {
			const glm::vec2 noisePos2d = glm::vec2(_noiseSeedOffsetX + region.getLowerX() + x, _noiseSeedOffsetZ + region.getLowerZ() + z);
			const float landscapeNoise = noise::Simplex::Noise2D(noisePos2d, 3, 0.1f, 0.01f);
			const float noiseNormalized = (landscapeNoise + 1.0f) * 0.5f;
			const float mountainNoise = noise::Simplex::Noise2D(noisePos2d, 2, 0.3f, 0.00075f);
			const float mountainNoiseNormalized = (mountainNoise + 1.0f) * 0.5f;
			const float mountainMultiplier = mountainNoiseNormalized * (mountainNoiseNormalized + 0.5f);
			const float n = glm::clamp(noiseNormalized * mountainMultiplier, 0.0f, 1.0f);
			const int ni = n * (MAX_TERRAIN_HEIGHT - 1);
			int y = 0;
			for (int h = lowerY; h < ni; ++h) {
				const Voxel voxel = _biomManager.getVoxelType(lowerX + x, h, lowerZ + z);
				chunk->setVoxel(x, y++, z, voxel);
				if (y >= height) {
					break;
				}
			}
		}
	}
	const glm::vec3 worldPos(lowerX, lowerY, lowerZ);
	if (_clientData && _biomManager.hasClouds(worldPos)) {
		createClouds(region, chunk, _random);
	}
	if (_biomManager.hasTrees(worldPos)) {
		createTrees(region, chunk, _random);
	}
}

void World::onFrame(long dt) {
}

}
