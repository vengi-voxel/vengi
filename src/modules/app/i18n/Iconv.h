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

#include "core/NonCopyable.h"
#include "core/String.h"

namespace app {

class IConv : public core::NonCopyable {
private:
	core::String _toCharset;
	core::String _fromCharset;
	void* _cd;

public:
	IConv();
	IConv(const core::String &fromcode, const core::String &tocode);
	~IConv();

	bool setCharsets(const core::String &fromcode, const core::String &tocode);
	core::String convert(const core::String &text);
};

} // namespace app
