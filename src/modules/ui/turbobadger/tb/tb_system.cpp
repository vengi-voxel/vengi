/**
 * @file
 */

#include "tb_system.h"

#include "tb_msg.h"
#include "tb_types.h"
#include <stdio.h>
#include "core/App.h"
#include "io/Filesystem.h"
#include <SDL.h>

namespace tb {

double TBSystem::GetTimeMS()
{
	Uint64 freq = SDL_GetPerformanceFrequency();
	Uint64 now = SDL_GetPerformanceCounter();
	return 1000. * ((double)now / (double)freq);
}

/** Reschedule the platform timer, or cancel it if fire_time is TB_NOT_SOON.
	If fire_time is 0, it should be fired ASAP.
	If force is true, it will ask the platform to schedule it again, even if
	the fire_time is the same as last time. */
void TBSystem::RescheduleTimer(double fire_time)
{
}

int TBSystem::GetLongClickDelayMS()
{
	return 500;
}

int TBSystem::GetPanThreshold()
{
	return 5 * GetDPI() / 96;
}

int TBSystem::GetPixelsPerLine()
{
	return 40 * GetDPI() / 96;
}

int TBSystem::GetDPI()
{
#if SDL_VERSION_ATLEAST(2,0,4)
	float ddpi;
	if (SDL_GetDisplayDPI(0, &ddpi, NULL, NULL))
	{
		return 96;
	}
	return (int)ddpi;
#else
	return 96;
#endif
}

void TBClipboard::Empty()
{
	SetText("");
}

bool TBClipboard::HasText()
{
	return SDL_HasClipboardText();
}

bool TBClipboard::SetText(const char *text)
{
	return (0 == SDL_SetClipboardText(text));
}

bool TBClipboard::GetText(TBStr &text)
{
	if (const char *str = SDL_GetClipboardText())
		return text.Set(str);
	return false;
}

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

} // namespace tb
