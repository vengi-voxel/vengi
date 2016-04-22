#include "WorldPersister.h"
#include <zlib.h>
#include "core/App.h"
#include "core/Common.h"
#include "core/String.h"
#include "core/ByteStream.h"
#include "Voxel.h"

namespace voxel {

#define WORLD_FILE_VERSION 1

std::string WorldPersister::getWorldName(const Region& region, long seed) const {
	return core::string::format("world_%li_%i_%i_%i.wld", seed, region.getCentreX(), region.getCentreY(), region.getCentreZ());
}

void WorldPersister::erase(TerrainContext& ctx, long seed) {
	const core::App* app = core::App::getInstance();
	const io::FilesystemPtr& filesystem = app->filesystem();
	const Region& region = ctx.region;
	const std::string& filename = getWorldName(region, seed);
	// TODO: filesystem->remove(filename);
}

bool WorldPersister::load(TerrainContext& ctx, long seed) {
	const core::App* app = core::App::getInstance();
	const io::FilesystemPtr& filesystem = app->filesystem();
	const Region& region = ctx.region;
	const std::string& filename = getWorldName(region, seed);
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

	core::ByteStream bs(fileLen);
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
				core_assert(voxelBuf.getSize() >= 1);
				const VoxelType material = voxelBuf.readByte();
				const Voxel& voxel = createVoxel(material);
				ctx.chunk->setVoxel(x, y, z, voxel);
			}
		}
	}
	return true;
}

bool WorldPersister::save(TerrainContext& ctx, long seed) {
	core::ByteStream voxelStream;
	const Region& region = ctx.region;
	const int width = region.getWidthInVoxels();
	const int height = region.getHeightInVoxels();
	const int depth = region.getDepthInVoxels();

	for (int z = 0; z < depth; ++z) {
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				const Voxel& voxel = ctx.chunk->getVoxel(x, y, z);
				core_assert(sizeof(VoxelType) == sizeof(uint8_t));
				voxelStream.addByte(voxel.getMaterial());
			}
		}
	}

	// save the stuff
	const std::string& filename = getWorldName(region, seed);
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
	Log::debug("Wrote file %s (%i)", filename.c_str(), voxelSize);
	return true;
}

}
