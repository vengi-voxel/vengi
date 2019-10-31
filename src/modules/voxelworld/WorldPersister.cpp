/**
 * @file
 */

#include "WorldPersister.h"
#include "voxel/PagedVolumeWrapper.h"
#include "voxel/Constants.h"
#include "core/App.h"
#include "core/io/Filesystem.h"
#include "core/Common.h"
#include "core/String.h"
#include "core/ByteStream.h"
#include <zlib.h>

namespace voxelworld {

#define WORLD_FILE_VERSION 1

std::string WorldPersister::getWorldName(const voxel::Region& region, long seed) const {
	return core::string::format("world_%li_%i_%i_%i.wld", seed, region.getLowerX(), region.getLowerY(), region.getLowerZ());
}

void WorldPersister::erase(const voxel::Region& region, long seed) {
	if (!_persist) {
		return;
	}
	core_trace_scoped(WorldPersisterErase);
#if 0
	voxel::PagedVolume::ChunkPtr chunk = ctx.getChunk();
	const core::App* app = core::App::getInstance();
	const io::FilesystemPtr& filesystem = app->filesystem();
	const voxel::Region& region = ctx.region;
	const std::string& filename = getWorldName(region, seed);
	// TODO: filesystem->remove(filename);
#endif
}

bool WorldPersister::load(voxel::PagedVolume::Chunk* chunk, long seed) {
	if (!_persist) {
		return false;
	}
	core_trace_scoped(WorldPersisterLoad);
	const core::App* app = core::App::getInstance();
	const io::FilesystemPtr& filesystem = app->filesystem();
	const voxel::Region& region = chunk->region();
	const std::string& filename = getWorldName(region, seed);
	const io::FilePtr& f = filesystem->open(filename);
	if (!f->exists()) {
		return false;
	}
	Log::trace("Try to load world %s", f->name().c_str());
	uint8_t *fileBuf;
	// TODO: load async, put world into state loading, and do the real loading in onFrame if the file is fully loaded
	const int fileLen = f->read((void **) &fileBuf);
	if (!fileBuf || fileLen <= 0) {
		Log::error("Failed to load the world from %s", f->name().c_str());
		return false;
	}
	std::unique_ptr<uint8_t[]> smartBuf(fileBuf);

	core::ByteStream bs(fileLen);
	bs.append(fileBuf, fileLen);
	int len;
	int version;
	bs.readFormat("ib", &len, &version);

	if (version != WORLD_FILE_VERSION) {
		Log::error("file %s has a wrong version number %i (expected %i)", f->name().c_str(), version, WORLD_FILE_VERSION);
		return false;
	}
	const int sizeLimit = 1024;
	if (len > 1000l * 1000l * sizeLimit) {
		Log::error("extracted memory would be more than %i MB for the file %s", sizeLimit, f->name().c_str());
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
				core_assert_msg(voxelBuf.getSize() >= 1, "Failed to load %s (x: %i, y: %i, z: %i)", f->name().c_str(), x, y, z);
				static_assert(sizeof(voxel::VoxelType) == sizeof(uint8_t), "Voxel type size changed");
				const voxel::VoxelType material = (voxel::VoxelType)voxelBuf.readByte();
				const uint8_t colorIndex = voxelBuf.readByte();
				const voxel::Voxel& voxel = createVoxel(material, colorIndex);
				chunk->setVoxel(x, y, z, voxel);
			}
		}
	}
	return true;
}

bool WorldPersister::save(voxel::PagedVolume::Chunk* chunk, long seed) {
	if (!_persist) {
		return false;
	}
	core_trace_scoped(WorldPersisterLoad);
	core::ByteStream voxelStream;
	const voxel::Region& region = chunk->region();
	const int width = region.getWidthInVoxels();
	const int height = region.getHeightInVoxels();
	const int depth = region.getDepthInVoxels();

	for (int z = 0; z < depth; ++z) {
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				const voxel::Voxel& voxel = chunk->voxel(x, y, z);
				static_assert(sizeof(voxel::VoxelType) == sizeof(uint8_t), "Voxel type size changed");
				voxelStream.addByte(std::enum_value(voxel.getMaterial()));
				voxelStream.addByte(voxel.getColor());
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
