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

#include "PluralForms.h"
#include "core/NonCopyable.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicStringMap.h"

namespace app {

using MsgStrs = core::DynamicArray<core::String, 1>;

/**
 * @brief A simple dictionary class that mimics gettext() behaviour. Each
 * Dictionary only works for a single language, for managing multiple
 * languages and .po files at once use the DictionaryManager.
 */
class Dictionary : public core::NonCopyable {
public:
	typedef core::DynamicStringMap<MsgStrs> Entries;
	typedef core::DynamicStringMap<Entries> CtxtEntries;
private:
	Entries _entries;

	CtxtEntries _ctxtEntries;

	core::String _charset;
	PluralForms _pluralForms;

	const char *translate(const Entries &dict, const char *msgid) const;
	const char *translatePlural(const Entries &dict, const char *msgid, const char *msgidplural, int num) const;

	bool _hasFallback = false;
	Dictionary *_fallback = nullptr;

public:
	/** Constructs a dictionary converting to the specified \a charset (default UTF-8) */
	Dictionary(const core::String &charset = "UTF-8");
	~Dictionary();

	/** Return the charset used for this dictionary */
	core::String getCharset() const;

	void setPluralForms(const PluralForms &);
	PluralForms getPluralForms() const;

	/** Translate the string \a msgid. */
	const char *translate(const char *msgid) const;

	size_t size() const;

	/**
	 * Translate the string \a msgid to its correct plural form, based
	 * on the number of items given by \a num. \a msgid_plural is \a msgid in
	 * plural form.
	 */
	const char *translatePlural(const char *msgid, const char *msgidplural, int num) const;

	/**
	 * Translate the string \a msgid that is in context \a msgctx. A
	 * context is a way to disambiguate msgids that contain the same
	 * letters, but different meaning. For example "exit" might mean to
	 * quit doing something or it might refer to a door that leads
	 * outside (i.e. 'Ausgang' vs 'Beenden' in german)
	 */
	const char *translateCtxt(const char *msgctxt, const char *msgid) const;

	const char *translateCtxtPlural(const char *msgctxt, const char *msgid, const char *msgidplural, int num) const;

	/**
	 * Add a translation from \a msgid to \a msgstr to the dictionary,
	 * where \a msgid is the singular form of the message, msgid_plural the
	 * plural form and msgstrs a table of translations. The right
	 * translation will be calculated based on the \a num argument to
	 * translate().
	 */
	void addTranslation(const core::String &msgid, const core::String &msgid_plural,
						MsgStrs &&msgstrs);
	void addTranslation(const core::String &msgctxt, const core::String &msgid, const core::String &msgid_plural,
						MsgStrs &&msgstrs);

	/**
	 * Add a translation from \a msgid to \a msgstr to the
	 * dictionary
	 */
	void addTranslation(core::String &&msgid, core::String &&msgstr);
	void addTranslation(core::String &&msgctxt, core::String &&msgid, core::String &&msgstr);

	/**
	 * Iterate over all messages, Func is of type:
	 * void func(const core::String& msgid, const MsgStrs& msgstrs)
	 */
	template<class Func>
	Func foreach (Func &&func) {
		for (Entries::iterator i = _entries.begin(); i != _entries.end(); ++i) {
			func(i->first, i->second);
		}
		return func;
	}

	template<class Func>
	Func foreach (Func &&func) const {
		for (Entries::iterator i = _entries.begin(); i != _entries.end(); ++i) {
			func(i->first, i->second);
		}
		return func;
	}

	void addFallback(Dictionary *fallback) {
		_hasFallback = true;
		_fallback = fallback;
	}

	/**
	 * Iterate over all messages with a context, Func is of type:
	 * void func(const core::String& ctxt, const core::String& msgid, const MsgStrs& msgstrs)
	 */
	template<class Func>
	Func foreachCtxt(Func func) {
		for (CtxtEntries::iterator i = _ctxtEntries.begin(); i != _ctxtEntries.end(); ++i) {
			for (Entries::iterator j = i->second.begin(); j != i->second.end(); ++j) {
				func(i->first, j->first, j->second);
			}
		}
		return func;
	}
};

} // namespace app
