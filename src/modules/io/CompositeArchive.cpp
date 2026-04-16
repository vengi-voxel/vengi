/**
 * @file
 */

#include "CompositeArchive.h"
#include "core/StringUtil.h"

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
	const core::String normalized = core::string::sanitizePath(file);
	for (const ArchivePtr &archive : _archives) {
		if (archive->exists(normalized)) {
			return true;
		}
	}
	return false;
}

bool CompositeArchive::exists(const core::Path &file) const {
	return exists(file.toString());
}

void CompositeArchive::list(const core::String &basePath, ArchiveFiles &out, const core::String &filter) const {
	for (const ArchivePtr &archive : _archives) {
		ArchiveFiles archiveFiles;
		archive->list(basePath, archiveFiles, filter);
		for (const auto &entry : archiveFiles) {
			bool duplicate = false;
			for (const auto &existing : out) {
				if (existing.fullPath == entry.fullPath) {
					duplicate = true;
					break;
				}
			}
			if (!duplicate) {
				out.push_back(entry);
			}
		}
	}
}

SeekableReadStream *CompositeArchive::readStream(const core::String &filePath) {
	const core::String normalized = core::string::sanitizePath(filePath);
	for (const ArchivePtr &archive : _archives) {
		SeekableReadStream *stream = archive->readStream(normalized);
		if (stream) {
			return stream;
		}
	}
	return nullptr;
}

SeekableWriteStream *CompositeArchive::writeStream(const core::String &filePath) {
	const core::String normalized = core::string::sanitizePath(filePath);
	for (const ArchivePtr &archive : _archives) {
		SeekableWriteStream *stream = archive->writeStream(normalized);
		if (stream) {
			return stream;
		}
	}
	return nullptr;
}

} // namespace io
