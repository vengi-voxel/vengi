/**
 * @file
 */

#include "TextProcessor.h"
#include "core/Var.h"
#include "util/KeybindingHandler.h"

namespace util {

bool replacePlaceholders(const KeyBindingHandler &handler, const core::String &str, char *buf, size_t bufSize) {
	int idx = 0;
	for (size_t i = 0u; i < str.size(); ++i) {
		const char *c = &str.c_str()[i];
		if (SDL_strncmp(c, "<cvar:", 6) == 0) {
			const char *l = SDL_strchr(c, '>');
			if (l != nullptr) {
				c += 6;
				const intptr_t len = l - c;
				const core::String name(c, len);
				core_assert((int)len == (int)name.size());
				const core::VarPtr &var = core::getVar(name);
				i += 6 + len;
				const core::String &value = var->strVal();
				const size_t remaining = bufSize - idx;
				SDL_strlcpy(&buf[idx], value.c_str(), remaining);
				idx += (int)value.size();
				if (idx >= (int)bufSize) {
					return false;
				}
				continue;
			}
		} else if (SDL_strncmp(c, "<cmd:", 5) == 0) {
			const char *l = SDL_strchr(c, '>');
			if (l != nullptr) {
				c += 5;
				const intptr_t len = l - c;
				const core::String name(c, len);
				core_assert((int)len == (int)name.size());
				const core::String &value = handler.getKeyBindingsString(name.c_str());
				i += 5 + len;
				const size_t remaining = bufSize - idx;
				SDL_strlcpy(&buf[idx], value.c_str(), remaining);
				idx += (int)value.size();
				if (idx >= (int)bufSize) {
					return false;
				}
				continue;
			}
		}
		buf[idx++] = *c;
		if (idx >= (int)bufSize) {
			return false;
		}
	}
	if (idx >= (int)bufSize) {
		return false;
	}
	buf[idx] = '\0';
	return true;
}

} // namespace util
