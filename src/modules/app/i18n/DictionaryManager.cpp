/**
 * @file
 */

// tinygettext - A gettext replacement that works directly on .po files
// Copyright (c) 2006 Ingo Ruhnke <grumbel@gmail.com>
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

#include "DictionaryManager.h"
#include "Language.h"
#include "POParser.h"
#include "app/Async.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FileStream.h"
#include "io/FilesystemEntry.h"

namespace app {

DictionaryManager::DictionaryManager(const io::FilesystemPtr &filesystem, const core::String &charset)
	: _charset(charset), _filesystem(filesystem) {
}

DictionaryManager::~DictionaryManager() {
	for (auto i = _dictionaries.begin(); i != _dictionaries.end(); ++i) {
		delete i->second;
	}
}

void DictionaryManager::clearCache() {
	for (auto i = _dictionaries.begin(); i != _dictionaries.end(); ++i) {
		delete i->second;
	}
	_dictionaries.clear();
	_languages.clear();

	_currentDict = nullptr;
}

Dictionary &DictionaryManager::getDictionary() {
	if (_currentDict) {
		return *_currentDict;
	}
	if (_currentLanguage) {
		_currentDict = &getDictionary(_currentLanguage);
		return *_currentDict;
	}
	return _emptyDict;
}

Dictionary &DictionaryManager::getDictionary(const Language &language) {
	core_assert(language);

	auto i = _dictionaries.find(language);
	if (i != _dictionaries.end()) {
		return *i->second;
	}
	// Dictionary for languages lang isn't loaded, so we load it
	Dictionary *dict = new Dictionary(_charset);
	Log::debug("Create dictionary for language: %s", language.str().c_str());
	_dictionaries.put(language, dict);

	for (auto p = _searchPath.begin(); p != _searchPath.end(); ++p) {
		core::DynamicArray<io::FilesystemEntry> files;
		if (!_filesystem->list(p->c_str(), files, "*.po")) {
			continue;
		}
		if (files.empty()) {
			Log::debug("no .po files found in: %s", p->c_str());
			continue;
		}

		core::String bestFilename;
		int bestScore = 0;

		for (const auto &file : files) {
			const core::String &fileLng = convertFilename2Language(file.name);
			Language poLanguage = Language::fromEnv(fileLng);

			if (!poLanguage) {
				Log::warn("%s: warning: ignoring, unknown language", file.name.c_str());
			} else {
				int score = Language::match(language, poLanguage);
				if (score > bestScore) {
					bestScore = score;
					bestFilename = file.name;
				}
			}
		}

		if (!bestFilename.empty()) {
			const core::String &pofile = core::string::path(*p, bestFilename);
			const io::FilePtr &in = _filesystem->open(pofile);
			if (!in) {
				Log::error("failure opening: %s", pofile.c_str());
			} else {
				Log::debug("Parsing po file %s", pofile.c_str());
				io::FileStream stream(in);
				io::BufferedReadWriteStream pofileStream(stream, stream.size());
				pofileStream.seek(0, SEEK_SET);
				POParser::parse(pofile, pofileStream, *dict);
			}
		} else {
			Log::debug("no matching .po file found for language: %s", language.getName().c_str());
		}
	}
	Log::debug("Dictionary for language: %s loaded with %i entries", language.str().c_str(), (int)dict->size());

	if (!language.getCountry().empty()) {
		dict->addFallback(&getDictionary(Language::fromSpec(language.getLanguage())));
	}
	return *dict;
}

Languages DictionaryManager::getLanguages() {
	if (!_languages.empty()) {
		return _languages;
	}
	_languages.push_back(Language::fromSpec("en", "GB"));
	for (SearchPath::iterator p = _searchPath.begin(); p != _searchPath.end(); ++p) {
		core::DynamicArray<io::FilesystemEntry> files;
		if (!_filesystem->list(p->c_str(), files, "*.po")) {
			continue;
		}
		for (const io::FilesystemEntry &file : files) {
			const core::String &fileLng = convertFilename2Language(file.name);
			Language lng = Language::fromEnv(fileLng);
			if (lng) {
				_languages.push_back(lng);
			}
		}
	}
	app::sort_parallel(_languages.begin(), _languages.end(), core::Less<Language>());
	return _languages;
}

void DictionaryManager::setLanguage(const Language &language) {
	if (_currentLanguage != language) {
		_currentLanguage = language;
		_currentDict = nullptr;
	}
}

Language DictionaryManager::getLanguage() const {
	return _currentLanguage;
}

void DictionaryManager::setCharset(const core::String &charset) {
	clearCache(); // changing charset invalidates cache
	_charset = charset;
}

void DictionaryManager::setUseFuzzy(bool t) {
	clearCache();
	_useFuzzy = t;
}

bool DictionaryManager::getUseFuzzy() const {
	return _useFuzzy;
}

void DictionaryManager::addDirectory(const core::String &pathname, bool precedence /* = false */) {
	if (core::find(_searchPath.begin(), _searchPath.end(), pathname) == _searchPath.end()) {
		clearCache(); // adding directories invalidates cache
		if (precedence) {
			_searchPath.insert(_searchPath.begin(), pathname);
		} else {
			_searchPath.push_back(pathname);
		}
	}
}

void DictionaryManager::removeDirectory(const core::String &pathname) {
	auto it = core::find(_searchPath.begin(), _searchPath.end(), pathname);
	if (it != _searchPath.end()) {
		clearCache(); // removing directories invalidates cache
		_searchPath.erase(it);
	}
}

core::String DictionaryManager::convertFilename2Language(const core::String &strIn) const {
	core::String s;
	if (strIn.substr(strIn.size() - 3, 3) == ".po") {
		s = strIn.substr(0, strIn.size() - 3);
	} else {
		s = strIn;
	}

	bool underscoreFound = false;
	for (unsigned int i = 0; i < s.size(); i++) {
		if (underscoreFound) {
			// If we get a non-alphanumerical character/
			// we are done (en_GB.UTF-8) - only convert
			// the 'gb' part ... if we ever get this kind
			// of filename.
			if (!SDL_isalpha(s[i])) {
				break;
			}
			s[i] = static_cast<char>(SDL_toupper(s[i]));
		} else {
			underscoreFound = s[i] == '_';
		}
	}
	return s;
}

} // namespace app
