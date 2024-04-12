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

#include "Dictionary.h"
#include "core/Log.h"

namespace app {

Dictionary::Dictionary(const core::String &charset_) : _charset(charset_) {
}

Dictionary::~Dictionary() {
}

core::String Dictionary::getCharset() const {
	return _charset;
}

void Dictionary::setPluralForms(const PluralForms &plural_forms_) {
	_pluralForms = plural_forms_;
}

PluralForms Dictionary::getPluralForms() const {
	return _pluralForms;
}

const char *Dictionary::translatePlural(const char *msgid, const char *msgidPlural, int num) const {
	return translatePlural(_entries, msgid, msgidPlural, num);
}

const char *Dictionary::translatePlural(const Entries &dict, const char *msgid, const char *msgidPlural,
										int count) const {
	auto it = dict.find(msgid);
	if (it != dict.end()) {
		unsigned int n = _pluralForms.getPlural(count);
		const auto &msgstrs = it->second;
		if (n >= msgstrs.size()) {
			Log::error("Plural translation not available (and not set to empty): '%s'", msgid);
			Log::error("Missing plural form: %i", n);
			return msgid;
		}

		if (!msgstrs[n].empty()) {
			return msgstrs[n].c_str();
		}

		if (count == 1) {
			// default to english rules
			return msgid;
		}

		return msgidPlural;
	}
	Log::debug("Couldn't translate: %s", msgid);
	Log::debug("Candidates:");
	for (const auto &e : dict) {
		Log::debug("'%s'", e->first.c_str());
	}

	if (count == 1) {
		// default to english rules
		return msgid;
	}

	return msgidPlural;
}

const char *Dictionary::translate(const char *msgid) const {
	return translate(_entries, msgid);
}

const char *Dictionary::translate(const Entries &dict, const char *msgid) const {
	auto i = dict.find(msgid);
	if (i != dict.end() && !i->second.empty()) {
		return i->second[0].c_str();
	}
	Log::debug("Couldn't translate: %s", msgid);

	if (_hasFallback) {
		return _fallback->translate(msgid);
	}
	return msgid;
}

const char *Dictionary::translateCtxt(const char *msgctxt, const char *msgid) const {
	auto i = _ctxtEntries.find(msgctxt);
	if (i != _ctxtEntries.end()) {
		return translate(i->second, msgid);
	}
	Log::debug("Couldn't translate: %s", msgid);
	return msgid;
}

const char *Dictionary::translateCtxtPlural(const char *msgctxt, const char *msgid, const char *msgidplural,
											int num) const {
	auto i = _ctxtEntries.find(msgctxt);
	if (i != _ctxtEntries.end()) {
		return translatePlural(i->second, msgid, msgidplural, num);
	}
	Log::debug("Couldn't translate: %s", msgid);
	if (num != 1) {
		// default to english
		return msgidplural;
	}
	return msgid;
}

void Dictionary::addTranslation(const core::String &msgid, const core::String &msgidPlural,
								const core::DynamicArray<core::String> &msgstrs) {
	auto iter = _entries.find(msgid);
	if (iter == _entries.end() || iter->second.empty()) {
		_entries.put(msgid, msgstrs);
	} else {
		Log::warn("collision in add_translation: '%s', '%s'", msgid.c_str(), msgidPlural.c_str());
	}
}

void Dictionary::addTranslation(const core::String &msgid, const core::String &msgstr) {
	auto iter = _entries.find(msgid);
	if (iter == _entries.end() || iter->second.empty()) {
		_entries.put(msgid, {msgstr});
	} else if (iter->second[0] != msgstr) {
		Log::warn("collision in add_translation: '%s', '%s'", msgid.c_str(), msgstr.c_str());
	}
}

void Dictionary::addTranslation(const core::String &msgctxt, const core::String &msgid, const core::String &msgidPlural,
								const core::DynamicArray<core::String> &msgstrs) {
	auto citer = _ctxtEntries.find(msgctxt);
	if (citer == _ctxtEntries.end()) {
		_ctxtEntries.emplace(msgctxt, {{msgid, msgstrs}});
	} else if (citer->value.find(msgid) == citer->value.end()) {
		citer->value.put(msgid, msgstrs);
	} else {
		Log::warn("collision in add_translation: '%s', '%s', '%s'", msgctxt.c_str(), msgid.c_str(),
				  msgidPlural.c_str());
	}
}

void Dictionary::addTranslation(const core::String &msgctxt, const core::String &msgid, const core::String &msgstr) {
	auto citer = _ctxtEntries.find(msgctxt);
	if (citer == _ctxtEntries.end()) {
		_ctxtEntries.emplace(msgctxt, {{msgid, {msgstr}}});
	} else if (citer->value.find(msgid) == citer->value.end()) {
		citer->value.emplace(msgid, {msgstr});
	} else {
		Log::warn("collision in add_translation: '%s', '%s'", msgctxt.c_str(), msgid.c_str());
	}
}

} // namespace app
