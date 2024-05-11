/**
 * @file
 */

#include "MemoryArchive.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryReadStream.h"
#include "io/Stream.h"

namespace io {

MemoryArchive::~MemoryArchive() {
	for (auto entry : _entries) {
		delete entry->second;
	}
	_entries.clear();
}

bool MemoryArchive::init(const core::String &path, io::SeekableReadStream *stream) {
	return true;
}

void MemoryArchive::shutdown() {
	_entries.clear();
}

bool MemoryArchive::add(const core::String &name, const uint8_t *data, size_t size) {
	auto iter = _entries.find(name);
	if (iter != _entries.end()) {
		return false;
	}
	MemoryReadStream memstream(data, size);
	_entries.put(name, new BufferedReadWriteStream(memstream, size));
	return true;
}

bool MemoryArchive::remove(const core::String &name) {
	auto iter = _entries.find(name);
	if (iter == _entries.end()) {
		return false;
	}
	_entries.erase(iter);
	return true;
}

SeekableWriteStream *MemoryArchive::writeStream(const core::String &filePath) {
	auto iter = _entries.find(filePath);
	if (iter == _entries.end()) {
		BufferedReadWriteStream *s = new BufferedReadWriteStream(512 * 1024);
		_entries.put(filePath, s);
		return new SeekableReadWriteStreamWrapper((io::SeekableWriteStream*)s);
	}
	return iter->second;
}

SeekableReadStream *MemoryArchive::readStream(const core::String &filePath) {
	auto iter = _entries.find(filePath);
	if (iter == _entries.end()) {
		return nullptr;
	}
	return new SeekableReadWriteStreamWrapper((io::SeekableReadStream*)iter->second);
}

} // namespace io
