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
#include <PolyVox/AStarPathfinder.h>
#include <PolyVox/CubicSurfaceExtractor.h>
#include <PolyVox/RawVolume.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL.h>
#include <zlib.h>

namespace voxel {

#define MAX_HEIGHT 256
#define WORLD_FILE_VERSION 1

// http://code.google.com/p/fortressoverseer/source/browse/Overseer/PolyVoxGenerator.cpp
World::World() :
		_volumeData(nullptr), _seed(0), _size(0), _noise(), _worldShapeNoise(), _threadPool(1), _rwLock("World") {
	_chunkSize = core::Var::get("cl_chunksize", "16", core::CV_READONLY);
}

World::~World() {
	if (_volumeData)
		delete _volumeData;
	_volumeData = nullptr;
}

int World::internalFindFloor(int x, int z) const {
	for (int i = MAX_HEIGHT - 1; i >= 0; i--) {
		const int material = getMaterial(x, i, z);
		if (material != AIR && material != CLOUDS) {
			return i + 1;
		}
	}
	return -1;
}

glm::ivec3 World::internalRandomPos(int border) const {
	const glm::ivec2& pos = randomPosWithoutHeight(border);
	const int y = internalFindFloor(pos.x, pos.y);
	return glm::ivec3(pos.x, y, pos.y);
}

glm::ivec2 World::randomPosWithoutHeight(int border) const {
	if (_size <= 2 * border) {
		Log::warn("Border %i exceeds size %i", border, _size);
		return glm::ivec2(0);
	}
	std::uniform_int_distribution<int> distribution(border, _size - border);
	const int x = distribution(_engine);
	const int z = distribution(_engine);
	return glm::ivec2(x, z);
}

glm::ivec3 World::randomPos(int border) const {
	const glm::ivec2& pos = randomPosWithoutHeight(border);
	const int y = findFloor(pos.x, pos.y);
	return glm::ivec3(pos.x, y, pos.y);
}

// Extract the surface for the specified region of the volume.
// The surface extractor outputs the mesh in an efficient compressed format which
// is not directly suitable for rendering.
void World::scheduleMeshExtraction(const glm::ivec2& p) {
	if (p.x < 0 || p.y < 0 || p.x >= _size || p.y >= _size) {
		Log::debug("skip mesh extraction for %i:%i (%i)", p.x, p.y, _size);
		return;
	}

	const int size = _chunkSize->intVal();
	const glm::ivec2& pos = getGridPos(p);
	if (_meshesExtracted.find(pos) != _meshesExtracted.end()) {
		Log::debug("mesh is already extracted for %i:%i (%i)", p.x, p.y, _size);
		return;
	}
	_meshesExtracted.insert(pos);

	const int delta = size - 1;
	_threadPool.enqueue([=] () {
		core_trace_scoped("MeshExtraction");
		const PolyVox::Vector3DInt32 mins(pos.x, 0, pos.y);
		const PolyVox::Vector3DInt32 maxs(mins.getX() + delta, MAX_HEIGHT, mins.getZ() + delta);
		const PolyVox::Region region(mins, maxs);
		DecodedMeshData data;
		{
			std::lock_guard<std::mutex> lock(_mutex);
			data.mesh = PolyVox::decodeMesh(PolyVox::extractCubicMesh(_volumeData, region));
		}

		data.translation = pos;
		core::ScopedWriteLock lock(_rwLock);
		_meshQueue.push_back(std::move(data));
	});
}

int World::findFloor(int x, int z) const {
	std::lock_guard<std::mutex> lock(_mutex);
	return internalFindFloor(x, z);
}

void World::allowReExtraction(const glm::ivec2& pos) {
	_meshesExtracted.erase(getGridPos(pos));
}

World::Result World::raycast(const glm::vec3& start, const glm::vec3& end, voxel::Raycast& raycast) {
	if (_volumeData == nullptr)
		return World::FAILED;
	std::lock_guard<std::mutex> lock(_mutex);
	PolyVox::RaycastResult result = PolyVox::raycastWithEndpoints(_volumeData, PolyVox::Vector3DFloat(start.x, start.y, start.z),
			PolyVox::Vector3DFloat(end.x, end.y, end.z), raycast);
	if (result == PolyVox::RaycastResults::Completed)
		return World::COMPLETED;
	return World::INTERUPTED;
}

bool World::findPath(const PolyVox::Vector3DInt32& start, const PolyVox::Vector3DInt32& end,
		std::list<PolyVox::Vector3DInt32>& listResult) {
	if (_volumeData == nullptr)
		return false;

	static auto f = [] (const voxel::WorldData* volData, const PolyVox::Vector3DInt32& v3dPos) {
		voxel::Voxel voxel = volData->getVoxel(v3dPos);
		return voxel.getDensity() != 0;
	};

	std::lock_guard<std::mutex> lock(_mutex);
	const PolyVox::AStarPathfinderParams<voxel::WorldData> params(_volumeData, start, end, &listResult, 1.0f, 10000,
			PolyVox::TwentySixConnected, std::bind(f, std::placeholders::_1, std::placeholders::_2));
	PolyVox::AStarPathfinder<voxel::WorldData> pf(params);
	// TODO: move into threadpool
	pf.execute();
	return true;
}

void World::destroy() {
	if (!_volumeData)
		return;
	std::lock_guard<std::mutex> lock(_mutex);
	if (!_volumeData)
		return;
	Log::info("flush the world");
	delete _volumeData;
	_volumeData = nullptr;
}

void World::createCirclePlane(const glm::ivec3& center, int width, int depth, double radius, const Voxel& voxel) {
	const double xRadius = (width - 1) / 2.0;
	const double zRadius = (depth - 1) / 2.0;
	const double minRadius = std::min(xRadius, zRadius);
	const double xRatio = xRadius / (float) minRadius;
	const double zRatio = zRadius / (float) minRadius;

	for (double z = -zRadius; z <= zRadius; ++z) {
		for (double x = -xRadius; x <= xRadius; ++x) {
			const double xP = x / xRatio;
			const double zP = z / zRatio;
			const double distance = sqrt(pow(xP, 2) + pow(zP, 2));
			if (distance < radius) {
				if (_volumeData->getEnclosingRegion().containsPoint(center.x + x, center.y, center.z + z))
					_volumeData->setVoxel(center.x + x, center.y, center.z + z, voxel);
			}
		}
	}
}

void World::createCube(const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel) {
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			for (int z = 0; z < depth; ++z) {
				if (_volumeData->getEnclosingRegion().containsPoint(pos.x + x, pos.y + y, pos.z + z)) {
					_volumeData->setVoxel(pos.x + x, pos.y + y, pos.z + z, voxel);
				}
			}
		}
	}
}

void World::createPlane(const glm::ivec3& pos, int width, int depth, const Voxel& voxel) {
	createCube(pos, width, 1, depth, voxel);
}

void World::createEllipse(const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel) {
	const double heightRadius = (height - 1.0) / 2.0;
	const double minDimension = std::min(width, depth);
	const double adjustedMinRadius = (minDimension - 1.0) / 2.0;
	const double heightFactor = heightRadius / adjustedMinRadius;
	for (double y = -heightRadius; y <= heightRadius; ++y) {
		const double adjustedHeight = abs(y / heightFactor);
		const double circleRadius = sqrt(pow(adjustedMinRadius + 0.5, 2.0) - pow(adjustedHeight, 2.0));
		const glm::ivec3 planePos(pos.x, pos.y + y, pos.z);
		createCirclePlane(planePos, width, depth, circleRadius, voxel);
	}
}

void World::createCone(const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel) {
	const double heightRadius = height - 0.5;
	const double minDimension = std::min(width, depth);
	const double minRadius = minDimension / 2.0;
	for (double y = 0.5; y <= heightRadius; y++) {
		const double percent = 1 - (y / height);
		const double circleRadius = percent * minRadius;
		const glm::ivec3 planePos(pos.x, pos.y + y, pos.z);
		createCirclePlane(planePos, width, depth, circleRadius, voxel);
	}
}

void World::createDome(const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel) {
	// TODO:
}

void World::addTree(const glm::ivec3& pos, TreeType type, int trunkHeight) {
	const int top = (int) pos.y + trunkHeight;
	const int sizeX = _volumeData->getWidth();
	const int sizeY = _volumeData->getDepth();
	const int sizeZ = _volumeData->getHeight();
	const Voxel voxel(TRUNK, Voxel::getMaxDensity());

	Log::trace("generate tree at %i:%i:%i", pos.x, pos.y, pos.z);

	for (int y = pos.y; y < top; ++y) {
		const int width = std::max(1, 3 - (y - pos.y));
		for (int x = pos.x - width; x < pos.x + width; ++x) {
			for (int z = pos.z - width; z < pos.z + width; ++z) {
				if ((x >= pos.x + 1 || x < pos.x - 1) && (z >= pos.z + 1 || z < pos.z - 1))
					continue;
				if (x < 0 || y < 0 || z < 0 || x >= sizeX || y >= sizeY || z >= sizeZ)
					continue;

				_volumeData->setVoxel(x, y, z, voxel);
			}
		}
	}

	const int width = 16;
	const int depth = 16;
	const int height = 12;
	const Voxel leavesVoxel(LEAVES, 1);
	if (type == TreeType::ELLIPSIS) {
		const int centerLeavesPos = top + height / 2;
		const glm::ivec3 leafesPos(pos.x, centerLeavesPos, pos.z);
		createEllipse(leafesPos, width, height, depth, leavesVoxel);
	} else if (type == TreeType::CONE) {
		const glm::ivec3 leafesPos(pos.x, top, pos.z);
		createCone(leafesPos, width, height, depth, leavesVoxel);
	} else if (type == TreeType::DOME) {
		const glm::ivec3 leafesPos(pos.x, top, pos.z);
		createDome(leafesPos, width, height, depth, leavesVoxel);
	}
}

void World::createTrees() {
	const int amount = 4;
	Log::debug("generate %i trees", amount);
	// TODO: don't place trees on top of other trees
	// TODO: don't use random positions, but decide on the material of some positions
	for (int i = 0; i < amount; ++i) {
		const glm::ivec3& pos = internalRandomPos(10);
		addTree(pos, TreeType::ELLIPSIS);
	}
	for (int i = 0; i < amount; ++i) {
		const glm::ivec3& pos = internalRandomPos(10);
		addTree(pos, TreeType::CONE);
	}
}

void World::createClouds() {
	const int amount = 4;
	Log::debug("generate %i clouds", amount);

	const Voxel voxel(CLOUDS, Voxel::getMinDensity());
	for (int i = 0; i < amount; ++i) {
		const int height = 10;
		const glm::ivec2& pos = randomPosWithoutHeight();
		glm::ivec3 cloudCenterPos(pos.x, _size - height, pos.y);
		createEllipse(cloudCenterPos, 10, height, 10, voxel);
		cloudCenterPos.x -= 5;
		cloudCenterPos.y -= 5 + i;
		createEllipse(cloudCenterPos, 20, height, 35, voxel);
	}
}

void World::createUnderground() {
	glm::ivec3 startPos(1, 1, 1);
	const Voxel voxel(DIRT, Voxel::getMaxDensity());
	createPlane(startPos, 10, 10, voxel);
}

bool World::load(long seed, util::IProgressMonitor* progressMonitor) {
	const core::App* app = core::App::getInstance();
	const io::FilesystemPtr& filesystem = app->filesystem();
	const std::string filename = "world-" + std::to_string(seed) + ".wld";
	const io::FilePtr& f = filesystem->open(filename);
	if (!f)
		return false;
	destroy();
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
	bs.readFormat("ibli", &len, &version, &_seed, &_size);

	if (progressMonitor)
		progressMonitor->init(_size * MAX_HEIGHT * _size);

	if (version != WORLD_FILE_VERSION) {
		Log::error("file %s has a wrong version number %i (expected %i)", f->getName().c_str(), version, WORLD_FILE_VERSION);
		return false;
	}
	if (_seed != seed) {
		Log::error("file %s has a wrong seed %li (expected %li)", f->getName().c_str(), _seed, seed);
		return false;
	}
	if (_size <= 0) {
		Log::error("file %s has invalid size: %i", f->getName().c_str(), _size);
		return false;
	}
	const int sizeLimit = 256;
	if (len > 1000l * 1000l * sizeLimit) {
		Log::error("extracted memory would be more than %i MB for the file %s", sizeLimit, f->getName().c_str());
		return false;
	}

	_engine.seed(_seed);
	Log::info("Loading a world with size %i from file %s,uncompressing to %i", _size, f->getName().c_str(), (int) len);

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

	const PolyVox::Region region(0, 0, 0, _size, MAX_HEIGHT, _size);
	WorldData* volumeData = new WorldData(region);
	const PolyVox::Vector3DInt32& lower = region.getLowerCorner();
	const PolyVox::Vector3DInt32& upper = region.getUpperCorner();
	const int lowerZ = lower.getZ();
	const int upperZ = upper.getZ();
	const int lowerY = lower.getY();
	const int upperY = upper.getY();
	const int lowerX = lower.getX();
	const int upperX = upper.getX();
	for (int z = lowerZ; z < upperZ; ++z) {
		for (int y = lowerY; y < upperY; ++y) {
			for (int x = lowerX; x < upperX; ++x) {
				core_assert(voxelBuf.getSize() >= 2);
				const uint8_t material = voxelBuf.readByte();
				const uint8_t density = voxelBuf.readByte();
				const Voxel voxel(material, density);
				volumeData->setVoxel(x, y, z, voxel);
				if (progressMonitor)
					progressMonitor->step();
			}
		}
	}
	if (progressMonitor)
		progressMonitor->done();
	_volumeData = volumeData;
	return true;
}

bool World::save(long seed) {
	if (_volumeData == nullptr) {
		Log::error("No world created yet");
		return false;
	}
	if (seed != _seed) {
		Log::error("Seeds don't match");
		return false;
	}
	core::ByteStream voxelStream;
	const PolyVox::Region region(0, 0, 0, _size, MAX_HEIGHT, _size);
	const PolyVox::Vector3DInt32& lower = region.getLowerCorner();
	const PolyVox::Vector3DInt32& upper = region.getUpperCorner();
	const int lowerZ = lower.getZ();
	const int upperZ = upper.getZ();
	const int lowerY = lower.getY();
	const int upperY = upper.getY();
	const int lowerX = lower.getX();
	const int upperX = upper.getX();
	for (int z = lowerZ; z < upperZ; ++z) {
		for (int y = lowerY; y < upperY; ++y) {
			for (int x = lowerX; x < upperX; ++x) {
				const Voxel& voxel = _volumeData->getVoxel(x, y, z);
				voxelStream.addByte(voxel.getMaterial());
				voxelStream.addByte(voxel.getDensity());
			}
		}
	}

	// save the stuff
	const std::string filename = "world-" + std::to_string(seed) + ".wld";
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
	final.addFormat("ibli", voxelSize, WORLD_FILE_VERSION, _seed, _size);
	final.append(compressedVoxelBuf, neededVoxelBufLen);
	if (!filesystem->write(filename, final.getBuffer(), final.getSize())) {
		Log::error("Failed to write file %s", filename.c_str());
		return false;
	}
	Log::info("Wrote file %s (%i)", filename.c_str(), voxelSize);
	return true;
}

void World::create(long seed, int size, util::IProgressMonitor* progressMonitor) {
	if (_seed == seed)
		return;
	_seed = seed;
	_engine.seed(_seed);
	_size = size;
	Log::info("Using seed %li with size %i", seed, size);
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_volumeData != nullptr) {
			Log::info("Create a new world");
			destroy();
		}

		_noise.setSeed(seed);
		_noise.init();
		const PolyVox::Region region(0, 0, 0, size - 1, MAX_HEIGHT - 1, size - 1);
		WorldData* volumeData = new WorldData(region);
		if (progressMonitor)
			progressMonitor->init(size * MAX_HEIGHT * size + 2);
		const PolyVox::Vector3DInt32& lower = region.getLowerCorner();
		const PolyVox::Vector3DInt32& upper = region.getUpperCorner();
		for (double z = lower.getZ(); z < upper.getZ(); ++z) {
			for (double x = lower.getX(); x < upper.getX(); ++x) {
				const int height = (_noise.get(x, z, 0, 256.0) + 1) * 128;

				for (double h = 0; h <= height; ++h) {
					if (progressMonitor) {
						progressMonitor->step();
					}
					const Voxel voxel(DIRT, DIRT);
					volumeData->setVoxel(x, h, z, voxel);
				}
			}
		}
		_volumeData = volumeData;
		createTrees();
		if (progressMonitor) {
			progressMonitor->step();
		}
		createClouds();
		if (progressMonitor) {
			progressMonitor->step();
			progressMonitor->done();
		}
	}
	core::App::getInstance()->eventBus()->publish(WorldCreatedEvent(this));
}

void World::onFrame(long dt) {
}

}
