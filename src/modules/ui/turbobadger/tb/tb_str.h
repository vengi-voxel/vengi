/**
 * @file
 */

#pragma once

#include "tb_types.h"
#include <string.h>

namespace tb {

/** Use as parameter for string length if you know the string is null terminated.
	Can be used in functions that support it. */
#define TB_ALL_TO_TERMINATION 2147483647

/** Some useful C-like functions that's missing in the standard. */
const char *stristr(const char *arg1, const char *arg2);

/** Simple string class that doesn't own or change the string pointer. */

class TBStrC
{
protected:
	char *s;
public:
	TBStrC(const char *str) : s(const_cast<char *>(str)) {}

	inline int length() const							{ return strlen(s); }
	inline bool isEmpty() const							{ return s[0] == 0; }

	inline int compare(const char* str) const			{ return strcmp(s, str); }
	inline bool equals(const char* str) const			{ return !strcmp(s, str); }

	inline char operator[](int n) const					{ return s[n]; }
	inline operator const char *() const				{ return s; }
	const char *c_str() const							{ return s; }
};

/** TBStr is a simple string class.
	It's a compact wrapper for a char array, and doesn't do any storage magic to
	avoid buffer copying or remember its length. It is intended as "final storage"
	of strings since its buffer is compact.

	Serious work on strings is better done using TBTempBuffer and then set on a TBStr for
	final storage (since TBTempBuffer is optimized for speed rather than being compact).

	It is guaranteed to have a valid pointer at all times. If uninitialized, emptied or on
	out of memory, its storage will be a empty ("") const string.
*/

class TBStr : public TBStrC
{
public:
	~TBStr();
	TBStr();
	TBStr(const TBStr &str);
	TBStr(const char* str);
	TBStr(const char* str, int len);

	bool set(const char* str, int len = TB_ALL_TO_TERMINATION);
	bool setFormatted(const char* format, ...);

	void clear();

	void remove(int ofs, int len);
	bool insert(int ofs, const char *ins, int ins_len = TB_ALL_TO_TERMINATION);
	bool append(const char *ins, int ins_len = TB_ALL_TO_TERMINATION)	{ return insert(strlen(s), ins, ins_len); }

	inline operator char *() const						{ return s; }
	char *c_str() const									{ return s; }
	const TBStr& operator = (const TBStr &str)			{ set(str); return *this; }
};

} // namespace tb
