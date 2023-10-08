/**
 * @file
 */

#pragma once

#include "core/String.h"

namespace util {

class KeyBindingHandler;

/**
 * @brief Replace special placeholders with the corresponding values
 *
 * @c <cvar:cvarname> will get replaced by the value of the core::Var with the name @c cvarname.
 * @c <cmd:cmdname> will get replaced by the key binding of the command with the name @c cmdname.
 * @param handler The key binding handler to use for the command bindings
 * @param str The string to process
 * @param buf The buffer to write the result to
 * @param bufSize The size of the result buffer
 */
bool replacePlaceholders(const KeyBindingHandler &handler, const core::String &str, char *buf, size_t bufSize);

}
