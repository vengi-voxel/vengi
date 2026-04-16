/**
 * @file
 */

#include "MemoryArchive.h"
#include "core/StringUtil.h"
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
}

bool MemoryArchive::exists(const core::String &file) const {
	const core::String normalized = core::string::sanitizePath(file);
	return _entries.find(normalized) != _entries.end();
}

void MemoryArchive::list(const core::String &basePath, ArchiveFiles &out, const core::String &filter) const {
	const core::String normalizedBase = core::string::sanitizePath(basePath);
	for (const auto &iter : _entries) {
		const core::String &name = iter->first;
		if (!normalizedBase.empty() && !core::string::startsWith(name, normalizedBase)) {
			continue;
		}
		const core::String filename = core::string::extractFilenameWithExtension(name);
		if (core::string::fileMatchesMultiple(filename.c_str(), filter.c_str())) {
			FilesystemEntry fse;
			fse.fullPath = name;
			fse.name = filename;
			fse.type = FilesystemEntry::Type::file;
			fse.size = iter->second->size();
			out.push_back(fse);
		}
	}
}

bool MemoryArchive::add(const core::String &name, const uint8_t *data, size_t size) {
	const core::String normalized = core::string::sanitizePath(name);
	auto iter = _entries.find(normalized);
	if (iter != _entries.end()) {
		return false;
	}
	MemoryReadStream memstream(data, size);
	_entries.put(normalized, new BufferedReadWriteStream(memstream, size));
	return true;
}

bool MemoryArchive::remove(const core::String &name) {
	const core::String normalized = core::string::sanitizePath(name);
	auto iter = _entries.find(normalized);
	if (iter == _entries.end()) {
		return false;
	}
	delete iter->second;
	_entries.erase(iter);
	return true;
}

SeekableWriteStream *MemoryArchive::writeStream(const core::String &filePath) {
	const core::String normalized = core::string::sanitizePath(filePath);
	auto iter = _entries.find(normalized);
	if (iter == _entries.end()) {
		BufferedReadWriteStream *s = new BufferedReadWriteStream(512 * 1024);
		_entries.put(normalized, s);
		return new SeekableReadWriteStreamWrapper((io::SeekableWriteStream *)s);
	}
	return new SeekableReadWriteStreamWrapper((io::SeekableWriteStream *)iter->second);
}

SeekableReadStream *MemoryArchive::readStream(const core::String &filePath) {
	const core::String normalized = core::string::sanitizePath(filePath);
	auto iter = _entries.find(normalized);
	if (iter == _entries.end()) {
		return nullptr;
	}
	iter->second->seek(0);
	return new SeekableReadWriteStreamWrapper((io::SeekableReadStream *)iter->second);
}

} // namespace io
