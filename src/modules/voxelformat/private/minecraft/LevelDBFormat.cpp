/**
 * @file
 */

#include "LevelDBFormat.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "leveldb/db.h"
#include "leveldb/zlib_compressor.h"

namespace voxelformat {

bool LevelDBFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
									  scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
									  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}

	// leveldb::DB *db = nullptr;
	// leveldb::Options options;
	// leveldb::ZlibCompressor compressor;
	// options.compressors[0] = &compressor;
	// leveldb::Status status = leveldb::DB::Open(options, filename.c_str(), &db);
	// if (!status.ok()) {
	// 	Log::error("Failed to open leveldb database: %s", status.ToString().c_str());
	// 	return false;
	// }
	// leveldb::ReadOptions readOptions;
	// readOptions.fill_cache = false;
	// readOptions.verify_checksums = false;
	// std::string value;
	// db->Get(readOptions, "leveldb", &value);

	return false;
}

} // namespace voxelformat
