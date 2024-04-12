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

#include "core/String.h"

namespace app {

typedef unsigned int (*PluralFunc)(int n);

class PluralForms {
private:
	unsigned int _nplural = 0;
	PluralFunc _plural = nullptr;

public:
	static PluralForms fromString(const core::String &str);

	PluralForms() {
	}

	PluralForms(unsigned int nplural, PluralFunc plural) : _nplural(nplural), _plural(plural) {
	}

	unsigned int getNPlural() const {
		return _nplural;
	}

	unsigned int getPlural(int n) const {
		if (_plural)
			return _plural(n);
		return 0;
	}

	bool operator==(const PluralForms &other) const {
		return _nplural == other._nplural && _plural == other._plural;
	}

	bool operator!=(const PluralForms &other) const {
		return !(*this == other);
	}

	explicit operator bool() const {
		return _plural != nullptr;
	}
};

} // namespace app
