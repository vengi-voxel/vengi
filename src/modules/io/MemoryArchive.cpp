/**
 * @file
 */

#include "MemoryArchive.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryReadStream.h"

namespace io {

bool MemoryArchive::init(const core::String &path, io::SeekableReadStream *stream) {
	return true;
}

void MemoryArchive::shutdown() {
	_entries.clear();
}

bool MemoryArchive::load(const core::String &filePath, io::SeekableWriteStream &out) {
	auto iter = _entries.find(filePath);
	if (iter == _entries.end()) {
		return false;
	}
	return out.write(iter->second->getBuffer(), iter->second->size()) == iter->second->size();
}

bool MemoryArchive::add(const core::String &name, const uint8_t *data, size_t size) {
	auto iter = _entries.find(name);
	if (iter != _entries.end()) {
		return false;
	}
	MemoryReadStream memstream(data, size);
	_entries.emplace(name, core::make_shared<BufferedReadWriteStream>(memstream, size));
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

SeekableWriteStreamPtr MemoryArchive::writeStream(const core::String &filePath) {
	auto iter = _entries.find(filePath);
	if (iter == _entries.end()) {
		_entries.emplace(filePath, core::make_shared<BufferedReadWriteStream>(512 * 1024));
	}
	iter = _entries.find(filePath);
	return iter->second;
}

SeekableReadStreamPtr MemoryArchive::readStream(const core::String &filePath) {
	auto iter = _entries.find(filePath);
	if (iter == _entries.end()) {
		return {};
	}
	return iter->second;
}

} // namespace io
