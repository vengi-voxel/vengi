/**
 * @file
 */

#include "tb_system.h"

#include "core/App.h"
#include "core/io/Filesystem.h"
#include "tb_msg.h"
#include "tb_types.h"
#include <SDL.h>
#include <stdio.h>

namespace tb {

#ifdef _APPLE_
const int TB_defaultDpi = 72;
#else
const int TB_defaultDpi = 96;
#endif

double TBSystem::getTimeMS() {
	Uint64 freq = SDL_GetPerformanceFrequency();
	Uint64 now = SDL_GetPerformanceCounter();
	return 1000. * ((double)now / (double)freq);
}

/** Reschedule the platform timer, or cancel it if fire_time is TB_NOT_SOON.
	If fire_time is 0, it should be fired ASAP.
	If force is true, it will ask the platform to schedule it again, even if
	the fire_time is the same as last time. */
void TBSystem::rescheduleTimer(double fireTime) {
}

int TBSystem::getLongClickDelayMS() {
	return 500;
}

int TBSystem::getPanThreshold() {
	return 5 * getDPI() / TB_defaultDpi;
}

int TBSystem::getPixelsPerLine() {
	return 40 * getDPI() / TB_defaultDpi;
}

int TBSystem::getDPI() {
#if SDL_VERSION_ATLEAST(2, 0, 4)
	float ddpi;
	if (SDL_GetDisplayDPI(0, &ddpi, NULL, NULL) != 0) {
		return TB_defaultDpi;
	}
	return (int)ddpi;
#else
	return TB_defaultDpi;
#endif
}

void TBClipboard::empty() {
	setText("");
}

bool TBClipboard::hasText() {
	return SDL_HasClipboardText() != 0U;
}

bool TBClipboard::setText(const char *text) {
	return (0 == SDL_SetClipboardText(text));
}

bool TBClipboard::getText(core::String &text) {
	if (const char *str = SDL_GetClipboardText()) {
		text = str;
		return true;
	}
	return false;
}

class File : public TBFile {
public:
	File(const io::FilePtr &file) : _file(file) {
	}

	virtual long size() {
		return _file->length();
	}
	virtual size_t read(void *buf, size_t elemSize, size_t count) {
		return _file->read(buf, elemSize, count);
	}

private:
	io::FilePtr _file;
};

// static
TBFile *TBFile::open(const char *filename, TBFileMode mode) {
	io::FilePtr f;
	switch (mode) {
	case MODE_READ:
		f = io::filesystem()->open(filename, io::FileMode::Read);
		break;
	default:
		break;
	}
	if (!f || !f->exists()) {
		return nullptr;
	}
	return new File(f);
}

} // namespace tb
