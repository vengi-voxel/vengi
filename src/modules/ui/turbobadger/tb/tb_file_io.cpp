/**
 * @file
 */

#include "tb_system.h"

#include "core/App.h"
#include "io/Filesystem.h"
#include <SDL.h>

namespace tb {

class File: public TBFile {
public:
	File(const io::FilePtr& file) :
			_file(file) {
	}

	virtual long Size() {
		return _file->length();
	}
	virtual size_t Read(void *buf, size_t elemSize, size_t count) {
		return _file->read(buf, elemSize, count);
	}
private:
	io::FilePtr _file;
};

// static
TBFile *TBFile::Open(const char *filename, TBFileMode mode) {
	io::FilePtr f;
	switch (mode) {
	case MODE_READ:
		f = core::App::getInstance()->filesystem()->open(filename,
				io::FileMode::Read);
		break;
	default:
		break;
	}
	if (!f) {
		return nullptr;
	}
	return new File(f);
}

}
