/**
 * @file
 */

#include "CachingArchive.h"
#include "core/StringUtil.h"

namespace io {

CachingArchive::CachingArchive(const ArchivePtr &archive) : _archive(archive) {
}

CachingArchive::~CachingArchive() {
	CachingArchive::shutdown();
}

void CachingArchive::fillCache() const {
	if (!_dirty) {
		return;
	}
	_cache.clear();
	_archive->list("", _cache, "");
	_dirty = false;
}

bool CachingArchive::init(const core::String &path, io::SeekableReadStream *stream) {
	_dirty = true;
	return _archive->init(path, stream);
}

void CachingArchive::shutdown() {
	_cache.clear();
	_dirty = true;
	if (_archive) {
		_archive->shutdown();
	}
}

bool CachingArchive::exists(const core::String &file) const {
	const core::String normalized = core::string::sanitizePath(file);
	fillCache();
	for (const auto &entry : _cache) {
		if (entry.fullPath == normalized) {
			return true;
		}
	}
	return false;
}

void CachingArchive::list(const core::String &basePath, ArchiveFiles &out, const core::String &filter) const {
	const core::String normalizedBase = core::string::sanitizePath(basePath);
	fillCache();
	for (const auto &entry : _cache) {
		if (!normalizedBase.empty() && !core::string::startsWith(entry.fullPath, normalizedBase)) {
			continue;
		}
		if (core::string::fileMatchesMultiple(entry.name.c_str(), filter.c_str())) {
			out.push_back(entry);
		}
	}
}

SeekableReadStream *CachingArchive::readStream(const core::String &filePath) {
	return _archive->readStream(filePath);
}

SeekableWriteStream *CachingArchive::writeStream(const core::String &filePath) {
	_dirty = true;
	return _archive->writeStream(filePath);
}

bool CachingArchive::write(const core::String &filePath, io::ReadStream &stream) {
	_dirty = true;
	return _archive->write(filePath, stream);
}

void CachingArchive::invalidate() {
	_dirty = true;
}

ArchivePtr openCachingArchive(const ArchivePtr &archive) {
	return core::make_shared<CachingArchive>(archive);
}

} // namespace io
