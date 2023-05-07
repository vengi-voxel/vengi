/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "core/collection/DynamicArray.h"
#include "io/FilesystemEntry.h"
#include "io/Stream.h"

namespace io {

using ArchiveFiles = core::DynamicArray<io::FilesystemEntry>;
using SeekableReadStreamPtr = core::SharedPtr<SeekableReadStream>;

/**
 * @ingroup IO
 * @see ZipArchive
 */
class Archive {
protected:
	ArchiveFiles _files;

public:
	const ArchiveFiles &files() const;
	virtual ~Archive() = default;

	/**
	 * @param[in] path
	 * @param[in] stream @c io::SeekableReadStream pointer can be @c nullptr
	 */
	virtual bool init(const core::String &path, io::SeekableReadStream *stream);
	virtual void shutdown();

	/**
	 * @param[in] filePath The relative filePath to the path the archive was initialized with. For example if the
	 * archive was initialized with @c "/data" and the file is @c "/data/level1/level2/file.txt" then the @c filePath is
	 * only @c "level1/level2/file.txt".
	 */
	virtual bool load(const core::String &filePath, io::SeekableWriteStream &out) = 0;
	virtual SeekableReadStreamPtr readStream(const core::String &filePath);
};

inline const ArchiveFiles &Archive::files() const {
	return _files;
}

using ArchivePtr = core::SharedPtr<Archive>;
ArchivePtr openArchive(const core::String &path, io::SeekableReadStream *stream);

} // namespace io
