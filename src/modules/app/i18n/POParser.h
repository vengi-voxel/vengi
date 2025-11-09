/**
 * @file
 */

// tinygettext - A gettext replacement that works directly on .po files
// Copyright (c) 2009 Ingo Ruhnke <grumbel@gmail.com>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgement in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#pragma once

#include "Iconv.h"
#include "core/NonCopyable.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Stream.h"

namespace app {

class Dictionary;

class POParser : public core::NonCopyable {
private:
	core::String _filename;
	io::SeekableReadStream &_in;
	Dictionary &_dict;
	bool _useFuzzy;
	bool _error = false;

	bool _eof = false;
	bool _big5 = false;

	int _lineNumber = 0;
	core::String _currentLine;

	IConv _conv;
	io::BufferedReadWriteStream _out{512};

	POParser(const core::String &filename, io::SeekableReadStream &in, Dictionary &dict, bool useFuzzy = true);
	~POParser();

	bool parseHeader(const core::String &header);
	bool parse();
	void nextLine();
	core::String getString(unsigned int skip);
	bool getStringLine(io::WriteStream &str, size_t skip);
	bool isEmptyLine();
	bool prefix(const char *);
	bool error(const char *msg);
	void warning(const char *msg);

public:
	/**
	 * @param filename name of the istream, only used in error messages
	 * @param in stream from which the PO file is read.
	 * @param dict dictionary to which the strings are written
	 */
	static bool parse(const core::String &filename, io::SeekableReadStream &in, Dictionary &dict);
	static bool pedantic;
};

} // namespace app
