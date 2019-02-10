/**
 * @file
 */

#include "tb_str.h"
#include "core/Assert.h"

namespace tb {

static const char *empty = "";
inline void safe_delete(char *&str)
{
	if (str != empty && str)
		SDL_free(str);
	str = const_cast<char*>(empty);
}

const char *stristr(const char *arg1, const char *arg2)
{
	const char *a, *b;

	for(;*arg1;arg1++)
	{
		a = arg1;
		b = arg2;
		while (SDL_toupper(*a++) == SDL_toupper(*b++))
			if (!*b)
				return arg1;
	}
	return nullptr;
}

// == TBStr ==========================================================

TBStr::TBStr()
	: TBStrC(empty)
{
}

TBStr::TBStr(const char* str)
	: TBStrC(str == empty ? empty : SDL_strdup(str))
{
	if (!s)
		s = const_cast<char*>(empty);
}

TBStr::TBStr(const TBStr &str)
	: TBStrC(str.s == empty ? empty : SDL_strdup(str.s))
{
	if (!s)
		s = const_cast<char*>(empty);
}

TBStr::TBStr(const char* str, int len)
	: TBStrC(empty)
{
	set(str, len);
}

TBStr::~TBStr()
{
	safe_delete(s);
}

bool TBStr::set(const char* str, int len)
{
	safe_delete(s);
	if (len == TB_ALL_TO_TERMINATION)
		len = SDL_strlen(str);
	if (char *new_s = (char *) SDL_malloc(len + 1))
	{
		s = new_s;
		SDL_memcpy(s, str, len);
		s[len] = 0;
		return true;
	}
	return false;
}

bool TBStr::setFormatted(const char* format, ...)
{
	safe_delete(s);
	if (!format)
		return true;
	va_list ap;
	int max_len = 64;
	char *new_s = nullptr;
	while (true)
	{
		if (char *tris_try_new_s = (char *) SDL_realloc(new_s, max_len))
		{
			new_s = tris_try_new_s;

			va_start(ap, format);
			int ret = SDL_vsnprintf(new_s, max_len, format, ap);
			va_end(ap);

			if (ret > max_len) // Needed size is known (+2 for termination and avoid ambiguity)
				max_len = ret + 2;
			else if (ret == -1 || ret >= max_len - 1) // Handle some buggy vsnprintf implementations.
				max_len *= 2;
			else // Everything fit for sure
			{
				s = new_s;
				return true;
			}
		}
		else
		{
			// Out of memory
			SDL_free(new_s);
			break;
		}
	}
	return false;
}

void TBStr::clear()
{
	safe_delete(s);
}

void TBStr::remove(int ofs, int len)
{
	core_assert(ofs >= 0 && (ofs + len <= (int)SDL_strlen(s)));
	if (!len)
		return;
	char *dst = s + ofs;
	char *src = s + ofs + len;
	while (*src != 0)
		*(dst++) = *(src++);
	*dst = *src;
}

bool TBStr::insert(int ofs, const char *ins, int insLen)
{
	int len1 = SDL_strlen(s);
	if (insLen == TB_ALL_TO_TERMINATION)
		insLen = SDL_strlen(ins);
	int newlen = len1 + insLen;
	if (char *news = (char *) SDL_malloc(newlen + 1))
	{
		SDL_memcpy(&news[0], s, ofs);
		SDL_memcpy(&news[ofs], ins, insLen);
		SDL_memcpy(&news[ofs + insLen], &s[ofs], len1 - ofs);
		news[newlen] = 0;
		safe_delete(s);
		s = news;
		return true;
	}
	return false;
}

} // namespace tb
