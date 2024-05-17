/**
 * @file
 */

#include "core/collection/StringMap.h"
#include "io/Archive.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FilesystemEntry.h"
#include "io/Stream.h"

namespace voxelformat {

class VXCArchive : public io::Archive {
public:
	core::StringMap<io::BufferedReadWriteStream *> _streams;

public:
	VXCArchive(io::ReadStream &stream) {
		uint32_t entries;
		stream.readUInt32(entries);
		for (uint32_t i = 0; i < entries; ++i) {
			char path[1024];
			stream.readString(sizeof(path), path, true);
			uint32_t fileSize;
			stream.readUInt32(fileSize);
			_streams.emplace(path, new io::BufferedReadWriteStream(stream, fileSize));
			io::FilesystemEntry entry;
			entry.size = fileSize;
			entry.name = path;
			entry.fullPath = entry.name;
			entry.type = io::FilesystemEntry::Type::file;
			_files.push_back(entry);
		}
	}

	virtual ~VXCArchive() {
		for (const auto &iter : _streams) {
			delete iter->second;
		}
	}

	io::SeekableReadStream *readStream(const core::String &filename) override {
		auto iter = _streams.find(filename);
		if (iter == _streams.end()) {
			return nullptr;
		}
		iter->second->seek(0);
		return new io::SeekableReadWriteStreamWrapper((io::SeekableReadStream *)iter->second);
	}
};

} // namespace voxelformat
