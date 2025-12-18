/**
 * @file
 */

#include "KeybindingParser.h"
#include "CustomButtonNames.h"
#include "command/Command.h"
#include "core/ArrayLength.h"
#include "core/BindingContext.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/Tokenizer.h"
#include "core/collection/DynamicArray.h"
#include "core/Log.h"
#include <SDL_keyboard.h>

namespace util {

void KeybindingParser::parseKeyAndCommand(core::String key, const core::String& command, const core::String &context) {
	int modifier = KMOD_NONE;
	core::BindingContext bindingContext = core::parseBindingContext(context);
	if (key.size() > 1 && key.contains(COMMAND_PRESSED)) {
		core::TokenizerConfig cfg;
		core::Tokenizer tok(cfg, key, COMMAND_PRESSED);
		const core::Tokens& line = tok.tokens();
		if (line.size() > 1) {
			for (const core::String& token : line) {
				const core::String& lower = token.toLower();
				if (lower == "shift") {
					modifier |= KMOD_SHIFT;
				} else if (lower == "left_shift") {
					modifier |= KMOD_LSHIFT;
				} else if (lower == "right_shift") {
					modifier |= KMOD_RSHIFT;
				} else if (lower == "alt") {
					modifier |= KMOD_ALT;
				} else if (lower == "left_alt") {
					modifier |= KMOD_LALT;
				} else if (lower == "right_alt") {
					modifier |= KMOD_RALT;
				} else if (lower == "ctrl") {
					modifier |= KMOD_CONTROL;
				} else if (lower == "left_ctrl") {
					modifier |= KMOD_LCONTROL;
				} else if (lower == "right_ctrl") {
					modifier |= KMOD_RCONTROL;
				} else if (lower == "gui") {
					modifier |= KMOD_GUI;
				} else if (lower == "left_gui") {
					modifier |= KMOD_LGUI;
				} else if (lower == "right_gui") {
					modifier |= KMOD_RGUI;
				} else {
					key = token;
					if (key.empty()) {
						key = COMMAND_PRESSED;
					}
				}
			}
		}
	}

	SDL_Keycode keyCode = SDLK_UNKNOWN;
	uint16_t count = 1u;
	for (int i = 0; i < lengthof(button::CUSTOMBUTTONMAPPING); ++i) {
		if (button::CUSTOMBUTTONMAPPING[i].name == key) {
			keyCode = button::CUSTOMBUTTONMAPPING[i].key;
			count = button::CUSTOMBUTTONMAPPING[i].count;
			break;
		}
	}
	if (keyCode == SDLK_UNKNOWN) {
		core::string::replaceAllChars(key, '_', ' ');
		keyCode = SDL_GetKeyFromName(key.c_str());
		if (keyCode == SDLK_UNKNOWN) {
#if defined __APPLE__ || defined __EMSCRIPTEN__
			// see Cocoa_InitKeyboard
			key = core::string::replaceAll(key, "alt", "option");
			key = core::string::replaceAll(key, "gui", "command");
			keyCode = SDL_GetKeyFromName(key.c_str());
			if (keyCode == SDLK_UNKNOWN)
#elif defined _WIN32
			// see WIN_InitKeyboard
			key = core::string::replaceAll(key, "gui", "windows");
			keyCode = SDL_GetKeyFromName(key.c_str());
			if (keyCode == SDLK_UNKNOWN)
#endif
			// TODO: Application -> Menu for linux and windows
			{
				_lastError = core::String::format("could not get a valid key code for %s (skip binding for %s): %s",
												  key.c_str(), command.c_str(), SDL_GetError());
				Log::warn("%s", _lastError.c_str());

				++_invalidBindings;
				return;
			}
		}
	}
	_bindings.insert(std::make_pair(keyCode, CommandModifierPair(command, modifier, count, bindingContext)));
}

KeybindingParser::KeybindingParser(const core::String& key, const core::String& binding, const core::String &context) :
		_invalidBindings(0) {
	parseKeyAndCommand(key, binding, context);
}

KeybindingParser::KeybindingParser(const core::String& bindings) :
		_invalidBindings(0) {
	core::DynamicArray<core::String> tokens;
	core::string::splitString(bindings, tokens, "\r\n");
	for (const core::String &line : tokens) {
		if (line[0] == '#' || line[0] == '/') {
			continue;
		}
		core::Tokenizer tok(line, " ");
		if (tok.size() == 3u) {
			const core::String key = tok.next();
			const core::String command = tok.next();
			const core::String context = tok.next();
			parseKeyAndCommand(key, command, context);
			continue;
		}

		_lastError = core::String::format("Found invalid keybindings line '%s'", line.c_str());
		Log::warn("%s", _lastError.c_str());
		++_invalidBindings;
	}
}

}
