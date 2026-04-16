/**
 * @file
 */

#include "MemoryArchive.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FilesystemEntry.h"
#include "io/MemoryReadStream.h"
#include "io/Stream.h"

namespace io {

MemoryArchive::~MemoryArchive() {
	MemoryArchive::shutdown();
}

bool MemoryArchive::init(const core::String &path, io::SeekableReadStream *stream) {
	return true;
}

void MemoryArchive::shutdown() {
	for (auto entry : _entries) {
		delete entry->second;
	}
	_entries.clear();
	Archive::shutdown();
}

bool MemoryArchive::add(const core::String &name, const uint8_t *data, size_t size) {
	auto iter = _entries.find(name);
	if (iter != _entries.end()) {
		return false;
	}
	MemoryReadStream memstream(data, size);
	_entries.put(name, new BufferedReadWriteStream(memstream, size));
	FilesystemEntry fse = createFilesystemEntry(name);
	fse.size = size;
	fse.type = FilesystemEntry::Type::file;
	_files.push_back(fse);
	return true;
}

bool MemoryArchive::remove(const core::String &name) {
	auto iter = _entries.find(name);
	if (iter == _entries.end()) {
		return false;
	}
	delete iter->second;
	_entries.erase(iter);
	_files.erase_if([&](const auto &e) { return e.fullPath == name; });
	return true;
}

SeekableWriteStream *MemoryArchive::writeStream(const core::String &filePath) {
	auto iter = _entries.find(filePath);
	if (iter == _entries.end()) {
		BufferedReadWriteStream *s = new BufferedReadWriteStream(512 * 1024);
		_entries.put(filePath, s);
		FilesystemEntry fse = createFilesystemEntry(filePath);
		fse.type = FilesystemEntry::Type::file;
		_files.push_back(fse);
		return new SeekableReadWriteStreamWrapper((io::SeekableWriteStream *)s);
	}
	return new SeekableReadWriteStreamWrapper((io::SeekableWriteStream *)iter->second);
}

SeekableReadStream *MemoryArchive::readStream(const core::String &filePath) {
	auto iter = _entries.find(filePath);
	if (iter == _entries.end()) {
		return nullptr;
	}
	iter->second->seek(0);
	return new SeekableReadWriteStreamWrapper((io::SeekableReadStream *)iter->second);
}

} // namespace io
