/**
 * @file
 */

#include "CompositeArchive.h"

namespace io {

CompositeArchive::~CompositeArchive() {
	CompositeArchive::shutdown();
}

void CompositeArchive::add(const ArchivePtr &archive) {
	_archives.push_back(archive);
}

bool CompositeArchive::init(const core::String &path, io::SeekableReadStream *stream) {
	return true;
}

void CompositeArchive::shutdown() {
	_archives.clear();
	Archive::shutdown();
}

bool CompositeArchive::exists(const core::String &file) const {
	for (const ArchivePtr &archive : _archives) {
		if (archive->exists(file)) {
			return true;
		}
	}
	return false;
}

bool CompositeArchive::exists(const core::Path &file) const {
	for (const ArchivePtr &archive : _archives) {
		if (archive->exists(file)) {
			return true;
		}
	}
	return false;
}

void CompositeArchive::list(const core::String &basePath, ArchiveFiles &out, const core::String &filter) const {
	for (const ArchivePtr &archive : _archives) {
		archive->list(basePath, out, filter);
	}
}

SeekableReadStream *CompositeArchive::readStream(const core::String &filePath) {
	for (const ArchivePtr &archive : _archives) {
		SeekableReadStream *stream = archive->readStream(filePath);
		if (stream) {
			return stream;
		}
	}
	return nullptr;
}

SeekableWriteStream *CompositeArchive::writeStream(const core::String &filePath) {
	for (const ArchivePtr &archive : _archives) {
		SeekableWriteStream *stream = archive->writeStream(filePath);
		if (stream) {
			return stream;
		}
	}
	return nullptr;
}

} // namespace io
