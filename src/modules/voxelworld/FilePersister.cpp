/**
 * @file
 */

#include "FilePersister.h"
#include "voxel/PagedVolumeWrapper.h"
#include "voxel/Constants.h"
#include "core/App.h"
#include "core/io/Filesystem.h"
#include "core/Common.h"
#include "core/StringUtil.h"
#include "core/Trace.h"
#include "core/ByteStream.h"
#include "core/Zip.h"

namespace voxelworld {

static core::String getWorldName(const voxel::Region& region, unsigned int seed) {
	return core::string::format("world_%u_%i_%i_%i.wld", seed, region.getLowerX(), region.getLowerY(), region.getLowerZ());
}

void FilePersister::erase(const voxel::Region& region, unsigned int seed) {
	core_trace_scoped(WorldPersisterErase);
#if 0
	voxel::PagedVolume::ChunkPtr chunk = ctx.getChunk();
	const io::FilesystemPtr& filesystem = io::filesystem();
	const voxel::Region& region = ctx.region;
	const core::String& filename = getWorldName(region, seed);
	// TODO: filesystem->remove(filename);
#endif
}

bool FilePersister::load(voxel::PagedVolume::Chunk* chunk, unsigned int seed) {
	core_trace_scoped(WorldPersisterLoad);
	const io::FilesystemPtr& filesystem = io::filesystem();
	const voxel::Region& region = chunk->region();
	const core::String& filename = getWorldName(region, seed);
	const io::FilePtr& f = filesystem->open(filename);
	if (!f->exists()) {
		return false;
	}
	Log::trace("Try to load world %s", f->name().c_str());
	uint8_t *fileBuf;
	const int fileLen = f->read((void **) &fileBuf);
	const bool success = loadCompressed(chunk, fileBuf, fileLen);
	delete[] fileBuf;
	return success;
}

bool FilePersister::save(voxel::PagedVolume::Chunk* chunk, unsigned int seed) {
	core_trace_scoped(WorldPersisterLoad);
	core::ByteStream final;
	if (!saveCompressed(chunk, final)) {
		return false;
	}
	const core::String& filename = getWorldName(chunk->region(), seed);
	const io::FilesystemPtr& filesystem = io::filesystem();

	if (!filesystem->write(filename, final.getBuffer(), final.getSize())) {
		Log::error("Failed to write file %s", filename.c_str());
		return false;
	}
	Log::debug("Wrote file %s (%i)", filename.c_str(), (int)final.getSize());
	return true;
}

}
