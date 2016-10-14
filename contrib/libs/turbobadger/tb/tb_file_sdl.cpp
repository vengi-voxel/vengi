// ================================================================================
// ==      This file is a part of Turbo Badger. (C) 2011-2014, Emil Segerås      ==
// ==                     See tb_core.h for more information.                    ==
// ================================================================================

#include "tb_system.h"

#ifdef TB_FILE_SDL

#include <SDL.h>

namespace tb {

class TBSDLFile : public TBFile
{
public:
	TBSDLFile(SDL_RWops *f) : file(f) {}
	virtual ~TBSDLFile() { SDL_RWclose(file); }

	virtual long Size()
	{
		long oldpos = SDL_RWtell(file);
		SDL_RWseek(file, 0, RW_SEEK_END);
		long num_bytes = SDL_RWtell(file);
		SDL_RWseek(file, oldpos, RW_SEEK_SET);
		return num_bytes;
	}
	virtual size_t Read(void *buf, size_t elemSize, size_t count)
	{
		return SDL_RWread(file, buf, elemSize, count);
	}
private:
	SDL_RWops *file;
};

// static
TBFile *TBFile::Open(const char *filename, TBFileMode mode)
{
	SDL_RWops *f = nullptr;
	switch (mode)
	{
	case MODE_READ:
		f = SDL_RWFromFile(filename, "rb");
#ifdef TB_RUNTIME_DEBUG_INFO
		if (!f) {
			TBDebugPrint("TBFile::Open, unable to open file '%s'\n", filename);
		}
#endif
		break;
	default:
		break;
	}
	if (!f) {
		return nullptr;
	}
	TBSDLFile *tbf = new TBSDLFile(f);
	if (!tbf) {
		SDL_RWclose(f);
	}
	return tbf;
}

} // namespace tb

#endif // TB_FILE_SDL
