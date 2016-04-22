#include "World.h"
#include "core/App.h"
#include "core/ByteStream.h"
#include "core/Var.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/Trace.h"
#include "io/File.h"
#include "LodCreator.h"
#include "core/Random.h"
#include "noise/SimplexNoise.h"
#include <SDL.h>
#include "polyvox/AStarPathfinder.h"
#include "polyvox/CubicSurfaceExtractor.h"
#include "polyvox/RawVolume.h"
#include "polyvox/Voxel.h"
#include <zlib.h>

namespace voxel {

#define WORLD_FILE_VERSION 1

void World::Pager::pageIn(const Region& region, PagedVolume::Chunk* chunk) {
	TerrainContext ctx;
	ctx.region = region;
	ctx.chunk = chunk;
	if (!_world.load(ctx)) {
		_world.create(ctx);
	}
}

void World::Pager::pageOut(const Region& region, PagedVolume::Chunk* chunk) {
#if 0
	TerrainContext ctx;
	ctx.region = region;
	ctx.chunk = chunk;
	_world.save(ctx);
#endif
}

// http://code.google.com/p/fortressoverseer/source/browse/Overseer/PolyVoxGenerator.cpp
World::World() :
		_pager(*this), _seed(0), _clientData(false), _threadPool(1), _rwLock("World"),
		_random(_seed), _noiseSeedOffsetX(0.0f), _noiseSeedOffsetZ(0.0f) {
	_chunkSize = core::Var::get(cfg::VoxelChunkSize, "64", core::CV_READONLY);
	_volumeData = new PagedVolume(&_pager, 256 * 1024 * 1024, _chunkSize->intVal());
	core_assert(_biomManager.addBiom(0, 100, createVoxel(Grass)));
	core_assert(_biomManager.addBiom(101, MAX_HEIGHT - 1, createVoxel(Grass)));
}

World::~World() {
}

glm::ivec2 World::randomPosWithoutHeight(const Region& region, int border) {
	const int w = region.getWidthInVoxels();
	const int d = region.getDepthInVoxels();
	core_assert(border < w);
	core_assert(border < d);
	const int x = _random.random(border, w - border);
	const int z = _random.random(border, d - border);
	return glm::ivec2(x, z);
}

glm::ivec3 World::randomPos() const {
	// TODO: where the heck is the randomness
	const glm::ivec2 pos(0, 0);
	const int y = findFloor(pos.x, pos.y);
	return glm::ivec3(pos.x, y, pos.y);
}

struct IsVoxelTransparent {
	bool operator()(const Voxel& voxel) const {
		return voxel.getMaterial() == Air;
	}
};

/**
 * @brief The criteria used here are that the voxel in front of the potential
 * quad should have a Air voxel while the voxel behind the potential quad
 * would have a voxel that is not Air.
 */
struct IsQuadNeeded {
	bool operator()(const Voxel& back, const Voxel& front, Voxel& materialToUse) {
		if (back.getMaterial() != Air && front.getMaterial() == Air) {
			materialToUse = back;
			return true;
		}
		return false;
	}
};

void World::calculateAO(const Region& region) {
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
				//voxel.setDensity(ao);
				_volumeData->setVoxel(nx, ny, nz, voxel);
			}
		}
	}
}

// Extract the surface for the specified region of the volume.
// The surface extractor outputs the mesh in an efficient compressed format which
// is not directly suitable for rendering.
bool World::scheduleMeshExtraction(const glm::ivec3& p) {
	if (_cancelThreads)
		return false;
	const glm::ivec3& pos = getGridPos(p);
	auto i = _meshesExtracted.find(pos);
	if (i != _meshesExtracted.end()) {
		Log::trace("mesh is already extracted for %i:%i:%i (%i:%i:%i - %i:%i:%i)", p.x, p.y, p.z, i->x, i->y, i->z, pos.x, pos.y, pos.z);
		return false;
	}
	Log::trace("mesh extraction for %i:%i:%i (%i:%i:%i)", p.x, p.y, p.z, pos.x, pos.y, pos.z);
	_meshesExtracted.insert(pos);

	_futures.push_back(_threadPool.enqueue([=] () {
		if (_cancelThreads)
			return;
		core_trace_scoped("MeshExtraction");
		const Region& region = getRegion(pos);
		DecodedMeshData data;
		{
			locked([&] () {
				// calculate ao
				calculateAO(region);
				const bool mergeQuads = true;
				data.mesh[0] = decodeMesh(extractCubicMesh(_volumeData, region, IsQuadNeeded(), mergeQuads));

				const uint32_t downScaleFactor = 2;
				for (data.numLods = 1; data.numLods < 2; ++data.numLods) {
					Region srcRegion = region;
					srcRegion.grow(downScaleFactor);

					const glm::ivec3& lowerCorner = srcRegion.getLowerCorner();
					glm::ivec3 upperCorner = srcRegion.getUpperCorner();

					upperCorner = upperCorner - lowerCorner;
					upperCorner = upperCorner / 2;
					upperCorner = upperCorner + lowerCorner;

					Region targetRegion(lowerCorner, upperCorner);

					RawVolume rawVolume(targetRegion);
					rescaleCubicVolume(_volumeData, srcRegion, &rawVolume, rawVolume.getEnclosingRegion());
					targetRegion.shrink(1);
					data.mesh[data.numLods] = decodeMesh(extractCubicMesh(&rawVolume, targetRegion, IsQuadNeeded(), mergeQuads));
				}
			});
		}

		data.translation = pos;
		core::ScopedWriteLock lock(_rwLock);
		_meshQueue.push_back(std::move(data));
	}));
	return true;
}

Region World::getRegion(const glm::ivec3& pos) const {
	const int size = _chunkSize->intVal();
	int deltaX = size - 1;
	int deltaY = size - 1;
	int deltaZ = size - 1;
	const glm::ivec3 mins(pos.x, pos.y, pos.z);
	const glm::ivec3 maxs(pos.x + deltaX, pos.y + deltaY, pos.z + deltaZ);
	const Region region(mins, maxs);
	return region;
}

void World::placeTree(const World::TreeContext& ctx) {
	const glm::ivec3 pos(ctx.pos.x, findFloor(ctx.pos.x, ctx.pos.y), ctx.pos.y);
	const Region& region = getRegion(getGridPos(pos));
	TerrainContext tctx;
	tctx.chunk = nullptr;
	tctx.region = region;
	addTree(tctx, pos, ctx.type, ctx.trunkHeight, ctx.trunkWidth, ctx.width, ctx.depth, ctx.height);
}

int World::findChunkFloor(int chunkHeight, PagedVolume::Chunk* chunk, int x, int z) {
	for (int i = chunkHeight - 1; i >= 0; i--) {
		const int material = chunk->getVoxel(x, i, z).getMaterial();
		if (isFloor(material)) {
			return i + 1;
		}
	}
	return -1;
}

int World::findFloor(int x, int z) const {
	for (int i = MAX_HEIGHT; i >= 0; i--) {
		const int material = getMaterial(x, i, z);
		if (isFloor(material)) {
			return i + 1;
		}
	}
	return -1;
}

bool World::allowReExtraction(const glm::ivec3& pos) {
	return _meshesExtracted.erase(getGridPos(pos)) != 0;
}

bool World::findPath(const glm::ivec3& start, const glm::ivec3& end,
		std::list<glm::ivec3>& listResult) {
	static auto f = [] (const voxel::PagedVolume* volData, const glm::ivec3& v3dPos) {
		const voxel::Voxel& voxel = volData->getVoxel(v3dPos);
		return voxel.getMaterial() != Air;
	};

	locked([&] () {
		const AStarPathfinderParams<voxel::PagedVolume> params(_volumeData, start, end, &listResult, 1.0f, 10000,
				TwentySixConnected, std::bind(f, std::placeholders::_1, std::placeholders::_2));
		AStarPathfinder<voxel::PagedVolume> pf(params);
		// TODO: move into threadpool
		pf.execute();
	});
	return true;
}

void World::destroy() {
	reset();
	_seed = 0l;
}

void World::reset() {
	_cancelThreads = true;
}

bool World::isValidChunkPosition(TerrainContext& ctx, const glm::ivec3& pos) const {
	if (ctx.chunk == nullptr) {
		return false;
	}

	if (pos.x < 0 || pos.x >= ctx.region.getWidthInVoxels())
		return false;
	if (pos.y < 0 || pos.y >= ctx.region.getHeightInVoxels())
		return false;
	if (pos.z < 0 || pos.z >= ctx.region.getDepthInVoxels())
		return false;
	return true;
}

void World::setVolumeVoxel(TerrainContext& ctx, const glm::ivec3& pos, const Voxel& voxel) {
	glm::ivec3 finalPos = pos;
	if (ctx.chunk != nullptr) {
		finalPos.x += ctx.region.getLowerX();
		finalPos.y += ctx.region.getLowerY();
		finalPos.z += ctx.region.getLowerZ();
	}
	_volumeData->setVoxel(finalPos.x, finalPos.y, finalPos.z, voxel);
	const glm::ivec3& gridpos = getGridPos(finalPos);
	ctx.dirty.insert(gridpos);
}

void World::createCirclePlane(TerrainContext& ctx, const glm::ivec3& center, int width, int depth, double radius, const Voxel& voxel) {
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
			const glm::ivec3 pos(center.x + x, center.y, center.z + z);
			if (isValidChunkPosition(ctx, pos)) {
				ctx.chunk->setVoxel(pos.x, pos.y, pos.z, voxel);
			} else {
				setVolumeVoxel(ctx, pos, voxel);
			}
		}
	}
}

void World::createCube(TerrainContext& ctx, const glm::ivec3& center, int width, int height, int depth, const Voxel& voxel) {
	const int w = width / 2;
	const int h = height / 2;
	const int d = depth / 2;
	for (int x = -w; x < width - w; ++x) {
		for (int y = -h; y < height - h; ++y) {
			for (int z = -d; z < depth - d; ++z) {
				const glm::ivec3 pos(center.x + x, center.y + y, center.z + z);
				if (isValidChunkPosition(ctx, pos)) {
					ctx.chunk->setVoxel(pos.x, pos.y, pos.z, voxel);
				} else {
					setVolumeVoxel(ctx, pos, voxel);
				}
			}
		}
	}
}

void World::createPlane(TerrainContext& ctx, const glm::ivec3& center, int width, int depth, const Voxel& voxel) {
	createCube(ctx, center, width, 1, depth, voxel);
}

void World::createEllipse(TerrainContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel) {
	const int heightLow = height / 2;
	const int heightHigh = height - heightLow;
	const double minDimension = std::min(width, depth);
	const double adjustedMinRadius = minDimension / 2.0;
	const double heightFactor = heightLow / adjustedMinRadius;
	for (int y = -heightLow; y <= heightHigh; ++y) {
		const double percent = glm::abs(y / heightFactor);
		const double circleRadius = glm::pow(adjustedMinRadius + 0.5, 2.0) - glm::pow(percent, 2.0);
		const glm::ivec3 planePos(pos.x, pos.y + y, pos.z);
		createCirclePlane(ctx, planePos, width, depth, circleRadius, voxel);
	}
}

void World::createCone(TerrainContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel) {
	const int heightLow = height / 2;
	const int heightHigh = height - heightLow;
	const double minDimension = std::min(width, depth);
	const double minRadius = minDimension / 2.0;
	for (int y = -heightLow; y <= heightHigh; ++y) {
		const double percent = 1.0 - ((y + heightLow) / double(height));
		const double circleRadius = glm::pow(percent * minRadius, 2.0);
		const glm::ivec3 planePos(pos.x, pos.y + y, pos.z);
		createCirclePlane(ctx, planePos, width, depth, circleRadius, voxel);
	}
}

void World::createDome(TerrainContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel) {
	const int heightLow = height / 2;
	const int heightHigh = height - heightLow;
	const double minDimension = std::min(width, depth);
	const double minRadius = minDimension / 2.0;
	const double heightFactor = height / (minDimension - 1.0) / 2.0;
	for (int y = -heightLow; y <= heightHigh; ++y) {
		const double percent = glm::abs((y + heightLow) / heightFactor);
		const double circleRadius = glm::pow(minRadius, 2.0) - glm::pow(percent, 2.0);
		const glm::ivec3 planePos(pos.x, pos.y + y, pos.z);
		createCirclePlane(ctx, planePos, width, depth, circleRadius, voxel);
	}
}

void World::addTree(TerrainContext& ctx, const glm::ivec3& pos, TreeType type, int trunkHeight, int trunkWidth, int width, int depth, int height) {
	int top = (int) pos.y + trunkHeight;
	if (type == TreeType::PINE) {
		top += height;
	}

	const int chunkHeight = ctx.region.getHeightInVoxels();

	const Voxel voxel = createVoxel(Wood);
	for (int y = pos.y; y < top; ++y) {
		const int trunkWidthY = trunkWidth + std::max(0, 2 - (y - pos.y));
		for (int x = pos.x - trunkWidthY; x < pos.x + trunkWidthY; ++x) {
			for (int z = pos.z - trunkWidthY; z < pos.z + trunkWidthY; ++z) {
				if ((x >= pos.x + trunkWidthY || x < pos.x - trunkWidthY) && (z >= pos.z + trunkWidthY || z < pos.z - trunkWidthY)) {
					continue;
				}
				glm::ivec3 finalPos(x, y, z);
				if (y == pos.y) {
					if (isValidChunkPosition(ctx, finalPos)) {
						finalPos.y = findChunkFloor(chunkHeight, ctx.chunk, x, z);
					} else {
						finalPos.y = findFloor(x, z);
					}
					if (finalPos.y < 0) {
						continue;
					}
				}
				if (isValidChunkPosition(ctx, finalPos)) {
					ctx.chunk->setVoxel(finalPos.x, finalPos.y, finalPos.z, voxel);
				} else {
					setVolumeVoxel(ctx, finalPos, voxel);
				}
			}
		}
	}

	const VoxelType leavesType = _random.random(Leaves1, Leaves10);
	const Voxel leavesVoxel = createVoxel(leavesType);
	const glm::ivec3 leafesPos(pos.x, top + height / 2, pos.z);
	if (type == TreeType::ELLIPSIS) {
		createEllipse(ctx, leafesPos, width, height, depth, leavesVoxel);
	} else if (type == TreeType::CONE) {
		createCone(ctx, leafesPos, width, height, depth, leavesVoxel);
	} else if (type == TreeType::PINE) {
		const int steps = std::max(1, height / 4);
		const int singleHeight = steps;
		const int stepWidth = width / steps;
		const int stepDepth = depth / steps;
		int currentWidth = stepWidth;
		int currentDepth = stepDepth;
		for (int i = 0; i < steps; ++i) {
			glm::ivec3 pineLeaves(pos.x, top - i * singleHeight, pos.z);
			createDome(ctx, pineLeaves, currentWidth, singleHeight, currentDepth, leavesVoxel);
			pineLeaves.y -= 1;
			createDome(ctx, pineLeaves, currentWidth + 1, singleHeight, currentDepth + 1, leavesVoxel);
			currentDepth += stepDepth;
			currentWidth += stepWidth;
		}
	} else if (type == TreeType::DOME) {
		createDome(ctx, leafesPos, width, height, depth, leavesVoxel);
	} else if (type == TreeType::CUBE) {
		createCube(ctx, leafesPos, width, height, depth, leavesVoxel);
		// TODO: use CreatePlane
		createCube(ctx, leafesPos, width + 2, height - 2, depth - 2, leavesVoxel);
		createCube(ctx, leafesPos, width - 2, height + 2, depth - 2, leavesVoxel);
		createCube(ctx, leafesPos, width - 2, height - 2, depth + 2, leavesVoxel);
	}
}

void World::createTrees(TerrainContext& ctx) {
	const Region& region = ctx.region;
	const int chunkHeight = region.getHeightInVoxels();
	for (int i = 0; i < 5; ++i) {
		const int rndValX = _random.random(1, region.getWidthInVoxels() - 1);
		// number should be even
		if (!(rndValX % 2)) {
			continue;
		}

		const int rndValZ = _random.random(1, region.getDepthInVoxels() - 1);
		// TODO: use a noise map to get the position
		glm::ivec3 pos(rndValX, -1, rndValZ);
		const int y = findChunkFloor(chunkHeight, ctx.chunk, pos.x, pos.z);
		const int height = _random.random(10, 14);
		const int trunkHeight = _random.random(5, 9);
		if (y < 0 || y >= MAX_HEIGHT -1  - height - trunkHeight) {
			continue;
		}

		pos.y = y;

		const int maxSize = 14;
		const int size = _random.random(12, maxSize);
		const int trunkWidth = 1;
		const TreeType treeType = (TreeType)_random.random(0, int(TreeType::MAX) - 1);
		addTree(ctx, pos, treeType, trunkHeight, trunkWidth, size, size, height);
	}
}

void World::createClouds(TerrainContext& ctx) {
	const int amount = 4;
	static const Voxel voxel = createVoxel(Cloud);
	for (int i = 0; i < amount; ++i) {
		const int height = 10;
		const glm::ivec2& pos = randomPosWithoutHeight(ctx.region, 20);
		glm::ivec3 chunkCloudCenterPos(pos.x, ctx.region.getHeightInVoxels() - height, pos.y);
		createEllipse(ctx, chunkCloudCenterPos, 10, height, 10, voxel);
		chunkCloudCenterPos.x -= 5;
		chunkCloudCenterPos.y -= 5 + i;
		createEllipse(ctx, chunkCloudCenterPos, 20, height, 20, voxel);
	}
}

void World::createUnderground(TerrainContext& ctx) {
	glm::ivec3 startPos(1, 1, 1);
	const Voxel voxel = createVoxel(Grass);
	createPlane(ctx, startPos, 10, 10, voxel);
}

std::string World::getWorldName(const Region& region) const {
	return core::string::format("world_%li_%i_%i_%i.wld", _seed, region.getCentreX(), region.getCentreY(), region.getCentreZ());
}

bool World::load(TerrainContext& ctx) {
	const core::App* app = core::App::getInstance();
	const io::FilesystemPtr& filesystem = app->filesystem();
	const Region& region = ctx.region;
	const std::string& filename = getWorldName(region);
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
				const Voxel voxel = createVoxel(material);
				ctx.chunk->setVoxel(x, y, z, voxel);
			}
		}
	}
	return true;
}

bool World::save(TerrainContext& ctx) {
	Log::info("Save chunk");
	core::ByteStream voxelStream;
	const Region& region = ctx.region;
	const int width = region.getWidthInVoxels();
	const int height = region.getHeightInVoxels();
	const int depth = region.getDepthInVoxels();

	for (int z = 0; z < depth; ++z) {
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				const Voxel& voxel = ctx.chunk->getVoxel(x, y, z);
				voxelStream.addByte(voxel.getMaterial());
			}
		}
	}

	// save the stuff
	const std::string& filename = getWorldName(region);
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

void World::create(TerrainContext& ctx) {
	const Region& region = ctx.region;
	Log::debug("Create new chunk at %i:%i:%i", region.getCentreX(), region.getCentreY(), region.getCentreZ());
	const int width = region.getWidthInVoxels();
	const int depth = region.getDepthInVoxels();
	const int height = region.getHeightInVoxels();
	const int lowerY = region.getLowerY();
	const int lowerX = region.getLowerX();
	const int lowerZ = region.getLowerZ();
	// TODO: kill me
	const core::VarPtr& plainTerrain = core::Var::get("voxel-plainterrain", "false");
	const bool plainTerrainBool = plainTerrain->boolVal();

	// TODO: the 2d noise doesn't neep the same resolution - we can optimize this a lot
	for (int z = 0; z < depth; ++z) {
		for (int x = 0; x < width; ++x) {
			const glm::vec2 noisePos2d = glm::vec2(_noiseSeedOffsetX + lowerX + x, _noiseSeedOffsetZ + lowerZ + z);
			const float landscapeNoise = noise::Simplex::Noise2D(noisePos2d, _ctx.landscapeNoiseOctaves,
					_ctx.landscapeNoisePersistence, _ctx.landscapeNoiseFrequency, _ctx.landscapeNoiseAmplitude);
			const float noiseNormalized = noise::norm(landscapeNoise);
			const float mountainNoise = noise::Simplex::Noise2D(noisePos2d, _ctx.mountainNoiseOctaves,
					_ctx.mountainNoisePersistence, _ctx.mountainNoiseFrequency, _ctx.mountainNoiseAmplitude);
			const float mountainNoiseNormalized = noise::norm(mountainNoise);
			const float mountainMultiplier = mountainNoiseNormalized * (mountainNoiseNormalized + 0.5f);
			const float n = glm::clamp(noiseNormalized * mountainMultiplier, 0.0f, 1.0f);
			const int ni = n * (MAX_TERRAIN_HEIGHT - 1);
			int y = 0;
			int start = lowerY;
			if (start == y) {
				++start;
				const Voxel& voxel = _biomManager.getVoxelType(lowerX + x, 0, lowerZ + z);
				ctx.chunk->setVoxel(x, 0, z, voxel);
			}
			if (plainTerrainBool) {
				for (int h = lowerY; h < ni; ++h) {
					const Voxel& voxel = _biomManager.getVoxelType(lowerX + x, h, lowerZ + z);
					ctx.chunk->setVoxel(x, y, z, voxel);
				}
			} else {
				for (int h = lowerY; h < ni; ++h) {
					const glm::vec3 noisePos3d = glm::vec3(noisePos2d.x, h, noisePos2d.y);
					const float noiseVal = noise::norm(noise::Simplex::Noise3D(noisePos3d, _ctx.caveNoiseOctaves,
							_ctx.caveNoisePersistence, _ctx.caveNoiseFrequency, _ctx.caveNoiseAmplitude));
					const float finalDensity = noiseNormalized + noise::norm(noiseVal);
					if (finalDensity > _ctx.caveDensityThreshold) {
						const Voxel& voxel = _biomManager.getVoxelType(lowerX + x, h, lowerZ + z);
						ctx.chunk->setVoxel(x, y, z, voxel);
					}
					if (++y >= height) {
						break;
					}
				}
			}
		}
	}
	const glm::vec3 worldPos(lowerX, lowerY, lowerZ);
	if (_clientData && _biomManager.hasClouds(worldPos)) {
		createClouds(ctx);
	}
	if (_biomManager.hasTrees(worldPos)) {
		createTrees(ctx);
	}
	for (const glm::ivec3& pos : ctx.dirty) {
		if (region.containsPoint(pos.x, pos.y, pos.z)) {
			continue;
		}
		if (!allowReExtraction(pos)) {
			continue;
		}
		scheduleMeshExtraction(pos);
	}
}

void World::cleanupFutures() {
	for (auto i = _futures.begin(); i != _futures.end();) {
		auto& future = *i;
		if (std::future_status::ready == future.wait_for(std::chrono::milliseconds(0))) {
			i = _futures.erase(i);
			continue;
		}
		break;
	}
}

void World::onFrame(long dt) {
	cleanupFutures();
	if (_cancelThreads) {
		if (!_futures.empty()) {
			return;
		}
		locked([this] () {
			_volumeData->flushAll();
			_ctx = WorldContext();
			_meshesExtracted.clear();
			_meshQueue.clear();
			Log::info("reset the world");
		});
		_cancelThreads = false;
	}
}

bool World::isReset() const {
	return _cancelThreads;
}

}
