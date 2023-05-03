/**
 * @file
 */

#include "core/collection/DynamicArray.h"
#include "io/FilesystemEntry.h"
#include "io/Stream.h"

namespace io {

using ArchiveFiles = core::DynamicArray<io::FilesystemEntry>;

/**
 * @ingroup IO
 * @see ZipArchive
 */
class Archive {
protected:
	ArchiveFiles _files;

public:
	const ArchiveFiles &files() const;

	virtual bool init(const core::String &path, io::SeekableReadStream *stream);
	virtual void shutdown();

	virtual bool load(const core::String &file, io::SeekableWriteStream &out) = 0;
};

inline const ArchiveFiles &Archive::files() const {
	return _files;
}

} // namespace io
