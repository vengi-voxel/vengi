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

#pragma once

#include "Dictionary.h"
#include "Language.h"
#include "core/collection/Buffer.h"
#include "io/Filesystem.h"

namespace app {

using Languages = core::Buffer<Language>;

/**
 * Manager class for dictionaries, you give it a bunch of directories
 * with .po files and it will then automatically load the right file
 * on demand depending on which language was set.
 */
class DictionaryManager : public core::NonCopyable {
private:
	typedef core::DynamicMap<Language, Dictionary *, 11, Language_hash> Dictionaries;
	Dictionaries _dictionaries;

	typedef core::DynamicArray<core::String, 4> SearchPath;
	SearchPath _searchPath;

	core::String _charset;
	bool _useFuzzy = true;

	Language _currentLanguage;
	Dictionary *_currentDict = nullptr;

	Dictionary _emptyDict;

	// available languages from po files found in the search paths
	Languages _languages;

	io::FilesystemPtr _filesystem;

	void clearCache();

public:
	DictionaryManager(const io::FilesystemPtr &filesystem, const core::String &charset_ = "UTF-8");
	~DictionaryManager();

	/**
	 * Return the currently active dictionary, if none is set, an empty
	 * dictionary is returned.
	 */
	Dictionary &getDictionary();

	/** Get dictionary for language */
	Dictionary &getDictionary(const Language &language);

	/** Set a language based on a four? letter country code */
	void setLanguage(const Language &language);

	/** returns the (normalized) country code of the currently used language */
	Language getLanguage() const;

	void setUseFuzzy(bool t);
	bool getUseFuzzy() const;

	/** Set a charset that will be set on the returned dictionaries */
	void setCharset(const core::String &charset);

	/**
	 * Add a directory to the search path for dictionaries, earlier
	 * added directories have higher priority then later added ones.
	 * Set @p precedence to true to invert this for a single addition.
	 */
	void addDirectory(const core::String &pathname, bool precedence = false);

	/** Remove a directory from the search path */
	void removeDirectory(const core::String &pathname);

	/** Return a set of the available languages in their country code */
	Languages getLanguages();

	/**
	 * This function converts a .po filename (e.g. zh_TW.po) into a language
	 * specification (zh_TW). On case insensitive file systems (think windows)
	 * the filename and therefore the country specification is lower case
	 * (zh_tw). It Converts the lower case characters of the country back to
	 * upper case, otherwise tinygettext does not identify the country
	 * correctly.
	 */
	core::String convertFilename2Language(const core::String &s_in) const;
};

} // namespace app
