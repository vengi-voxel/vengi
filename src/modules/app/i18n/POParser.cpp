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

#include "POParser.h"
#include "Dictionary.h"
#include "Iconv.h"
#include "PluralForms.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Stream.h"

namespace app {

bool POParser::pedantic = true;

bool POParser::parse(const core::String &filename, io::SeekableReadStream &in, Dictionary &dict) {
	POParser parser(filename, in, dict);
	return parser.parse();
}

POParser::POParser(const core::String &filename, io::SeekableReadStream &in, Dictionary &dict, bool useFuzzy)
	: _filename(filename), _in(in), _dict(dict), _useFuzzy(useFuzzy) {
}

POParser::~POParser() {
}

void POParser::warning(const core::String &msg) {
	Log::warn("%s:%i: %s", _filename.c_str(), _lineNumber, msg.c_str());
}

bool POParser::error(const core::String &msg) {
	Log::error("%s:%i: %s", _filename.c_str(), _lineNumber, msg.c_str());
	_error = true;
	return false;
}

void POParser::nextLine() {
	_lineNumber += 1;
	if (!_in.readLine(_currentLine)) {
		_eof = true;
		_currentLine.clear();
	}
}

bool POParser::getStringLine(io::WriteStream &out, size_t skip) {
	if (skip + 1 >= static_cast<unsigned int>(_currentLine.size())) {
		return error("unexpected end of line");
	}

	if (_currentLine[skip] != '"') {
		return error("expected start of string '\"'");
	}

	size_t i;
	for (i = skip + 1; _currentLine[i] != '\"'; ++i) {
		if (_big5 && static_cast<unsigned char>(_currentLine[i]) >= 0x81 &&
			static_cast<unsigned char>(_currentLine[i]) <= 0xfe) {
			out.writeUInt8(_currentLine[i]);

			i += 1;

			if (i >= _currentLine.size()) {
				return error("invalid big5 encoding");
			}

			out.writeUInt8(_currentLine[i]);
		} else if (i >= _currentLine.size()) {
			return error("unexpected end of string");
		} else if (_currentLine[i] == '\\') {
			i += 1;

			if (i >= _currentLine.size())
				return error("unexpected end of string in handling '\\'");

			switch (_currentLine[i]) {
			case 'a':
				out.writeUInt8('\a');
				break;
			case 'b':
				out.writeUInt8('\b');
				break;
			case 'v':
				out.writeUInt8('\v');
				break;
			case 'n':
				out.writeUInt8('\n');
				break;
			case 't':
				out.writeUInt8('\t');
				break;
			case 'r':
				out.writeUInt8('\r');
				break;
			case '"':
				out.writeUInt8('"');
				break;
			case '\\':
				out.writeUInt8('\\');
				break;
			default:
				Log::warn("unhandled escape: %s: %i: '%c'", _filename.c_str(), _lineNumber, _currentLine[i]);
				out.writeUInt8(_currentLine[i - 1]);
				out.writeUInt8(_currentLine[i]);
				break;
			}
		} else {
			out.writeUInt8(_currentLine[i]);
		}
	}

	// process trailing garbage in line and warn if there is any
	for (i = i + 1; i < _currentLine.size(); ++i)
		if (!core::string::isspace(_currentLine[i])) {
			warning("unexpected garbage after string ignoren");
			break;
		}

	return true;
}

core::String POParser::getString(unsigned int skip) {
	io::BufferedReadWriteStream out;

	if (skip + 1 >= static_cast<unsigned int>(_currentLine.size())) {
		error("unexpected end of line");
		return "";
	}

	if (_currentLine[skip] == ' ' && _currentLine[skip + 1] == '"') {
		if (!getStringLine(out, skip + 1)) {
			return "";
		}
	} else {
		if (pedantic) {
			warning("keyword and string must be seperated by a single space");
		}

		for (;;) {
			if (skip >= static_cast<unsigned int>(_currentLine.size())) {
				error("unexpected end of line");
				return "";
			} else if (_currentLine[skip] == '\"') {
				if (!getStringLine(out, skip)) {
					return "";
				}
				break;
			} else if (!core::string::isspace(_currentLine[skip])) {
				error("string must start with '\"'");
				return "";
			} else {
				// skip space
			}

			skip += 1;
		}
	}

next:
	nextLine();
	for (size_t i = 0; i < _currentLine.size(); ++i) {
		if (_currentLine[i] == '"') {
			if (i == 1) {
				if (pedantic) {
					warning("leading whitespace before string");
				}
			}

			if (!getStringLine(out, i)) {
				return "";
			}
			goto next;
		} else if (core::string::isspace(_currentLine[i])) {
			// skip
		} else {
			break;
		}
	}
	out.seek(0);
	core::String str;
	out.readString(out.size(), str, false);
	return str;
}

static bool hasPrefix(const core::String &lhs, const core::String &rhs) {
	if (lhs.size() < rhs.size())
		return false;
	return lhs.compare(0, rhs.size(), rhs) == 0;
}

bool POParser::parseHeader(const core::String &header) {
	core::String fromCharset;
	size_t start = 0;
	for (size_t i = 0; i < header.size(); ++i) {
		if (header[i] != '\n') {
			continue;
		}
		const core::String &line = header.substr(start, i - start);

		if (hasPrefix(line, "Content-Type:")) {
			// from_charset = line.substr(len);
			const size_t len = strlen("Content-Type: text/plain; charset=");
			if (line.compare(0, len, "Content-Type: text/plain; charset=") == 0) {
				fromCharset = line.substr(len).toUpper();
			} else {
				warning("malformed Content-Type header");
			}
		} else if (hasPrefix(line, "Plural-Forms:")) {
			const PluralForms &pluralForms = PluralForms::fromString(line);
			if (!pluralForms) {
				warning("unknown Plural-Forms given");
			} else {
				if (!_dict.getPluralForms()) {
					_dict.setPluralForms(pluralForms);
				} else {
					if (_dict.getPluralForms() != pluralForms) {
						warning("Plural-Forms missmatch between .po file and dictionary");
					}
				}
			}
		}
		start = i + 1;
	}

	if (fromCharset.empty() || fromCharset == "CHARSET") {
		warning("charset not specified for .po, fallback to utf-8");
		fromCharset = "UTF-8";
	} else if (fromCharset == "BIG5") {
		_big5 = true;
	}

	_conv.setCharsets(fromCharset, _dict.getCharset());
	return true;
}

bool POParser::isEmptyLine() {
	if (_currentLine.empty()) {
		return true;
	}
	if (_currentLine[0] == '#') { // handle comments as empty lines
		return (_currentLine.size() == 1 || (_currentLine.size() >= 2 && core::string::isspace(_currentLine[1])));
	}
	for (auto i = _currentLine.begin(); i != _currentLine.end(); ++i) {
		if (!core::string::isspace(*i)) {
			return false;
		}
	}
	return true;
}

bool POParser::prefix(const char *prefixStr) {
	return _currentLine.compare(0, strlen(prefixStr), prefixStr) == 0;
}

bool POParser::parse() {
	nextLine();

	// skip UTF-8 intro that some text editors produce
	// see http://en.wikipedia.org/wiki/Byte-order_mark
	if (_currentLine.size() >= 3 && _currentLine[0] == static_cast<char>(0xef) &&
		_currentLine[1] == static_cast<char>(0xbb) && _currentLine[2] == static_cast<char>(0xbf)) {
		_currentLine = _currentLine.substr(3);
	}

	// Parser structure
	while (!_eof) {
		bool fuzzy = false;
		bool hasMsgctxt = false;
		core::String msgctxt;
		core::String msgid;

		while (prefix("#")) {
			if (_currentLine.size() >= 2 && _currentLine[1] == ',') {
				// FIXME: Rather simplistic hunt for fuzzy flag
				if (_currentLine.find("fuzzy", 2) != core::String::npos)
					fuzzy = true;
			}

			nextLine();
		}

		if (!isEmptyLine()) {
			if (prefix("msgctxt")) {
				hasMsgctxt = true;
				msgctxt = getString(7);
				if (_error) {
					return false;
				}
			}

			if (prefix("msgid")) {
				msgid = getString(5);
				if (_error) {
					return false;
				}
			} else {
				return error("expected 'msgid'");
			}

			if (prefix("msgid_plural")) {
				core::String msgid_plural = getString(12);
				if (_error) {
					return false;
				}
				core::DynamicArray<core::String> msgstr_num;
				bool saw_nonempty_msgstr = false;

			next:
				if (isEmptyLine()) {
					if (msgstr_num.empty()) {
						return error("expected 'msgstr[N] (0 <= N <= 9)'");
					}
				} else if (prefix("msgstr[") && _currentLine.size() > 8 && SDL_isdigit(_currentLine[7]) &&
						   _currentLine[8] == ']') {
					unsigned int number = static_cast<unsigned int>(_currentLine[7] - '0');
					core::String msgstr = getString(9);
					if (_error) {
						return false;
					}

					if (!msgstr.empty()) {
						saw_nonempty_msgstr = true;
					}

					if (number >= msgstr_num.size()) {
						msgstr_num.resize(number + 1);
					}

					msgstr_num[number] = _conv.convert(msgstr);
					goto next;
				} else {
					return error("expected 'msgstr[N]'");
				}

				if (!isEmptyLine()) {
					return error("expected 'msgstr[N]' or empty line");
				}

				if (saw_nonempty_msgstr) {
					if (_useFuzzy || !fuzzy) {
						if (!_dict.getPluralForms()) {
							warning("msgstr[N] seen, but no Plural-Forms given");
						} else if (msgstr_num.size() != _dict.getPluralForms().getNPlural()) {
							warning("msgstr[N] count doesn't match Plural-Forms.nplural");
						}

						if (hasMsgctxt) {
							_dict.addTranslation(msgctxt, msgid, msgid_plural, msgstr_num);
						} else {
							_dict.addTranslation(msgid, msgid_plural, msgstr_num);
						}
					}
				}
			} else if (prefix("msgstr")) {
				core::String msgstr = getString(6);
				if (_error) {
					return false;
				}

				if (msgid.empty()) {
					if (!parseHeader(msgstr)) {
						return false;
					}
				} else if (!msgstr.empty()) {
					if (_useFuzzy || !fuzzy) {
						if (hasMsgctxt) {
							_dict.addTranslation(msgctxt, msgid, _conv.convert(msgstr));
						} else {
							_dict.addTranslation(msgid, _conv.convert(msgstr));
						}
					}
				}
			} else {
				return error("expected 'msgstr' or 'msgid_plural'");
			}
		}

		if (!_eof && !isEmptyLine()) {
			return error("expected empty line");
		}

		nextLine();
	}
	return true;
}

} // namespace app
