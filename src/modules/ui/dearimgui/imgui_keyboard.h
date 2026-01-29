// License: MIT
// Copyright (c) 2026 Martin Gerhardy
//
// https://github.com/mgerhardy/imgui_keyboard
//
// The MIT License (MIT)
//
// Copyright (c) 2023 Martin Gerhardy
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "imgui.h"

namespace ImKeyboard {

enum ImGuiKeyboardLayout_ {
	ImGuiKeyboardLayout_Qwerty,		 // ANSI
	ImGuiKeyboardLayout_Qwertz,		 // ANSI
	ImGuiKeyboardLayout_Azerty,		 // ANSI
	ImGuiKeyboardLayout_Colemak,	 // ANSI
	ImGuiKeyboardLayout_Dvorak,		 // ANSI
	ImGuiKeyboardLayout_NumericPad,
	ImGuiKeyboardLayout_QwertyISO,	 // ISO (UK/International)
	ImGuiKeyboardLayout_QwertzISO,	 // ISO (German)
	ImGuiKeyboardLayout_AzertyISO,	 // ISO (French)
	ImGuiKeyboardLayout_AppleANSI,	 // Apple ANSI (US)
	ImGuiKeyboardLayout_AppleISO,	 // Apple ISO (UK/International)

	ImGuiKeyboardLayout_Count
};
typedef int ImGuiKeyboardLayout;

enum ImGuiKeyboardFlags_ {
	ImGuiKeyboardFlags_None = 0,
	ImGuiKeyboardFlags_ShowPressed = 1 << 0,	// Highlight keys that are currently pressed
	ImGuiKeyboardFlags_NoShiftLabels = 1 << 1,	// Don't show shift labels when Shift is pressed
	ImGuiKeyboardFlags_ShowBothLabels = 1 << 2, // Always show both normal and shift labels (shift label below)
	ImGuiKeyboardFlags_ShowIcons = 1 << 3,		// Show icons instead of text (Windows logo, arrow triangles)
	ImGuiKeyboardFlags_NoNumpad = 1 << 4,		// Skip rendering the numeric keypad
	ImGuiKeyboardFlags_Recordable = 1 << 5,		// Enable key recording for keybinding selection (click or press keys)
};
typedef int ImGuiKeyboardFlags;

// Style colors for the keyboard widget
enum ImGuiKeyboardCol_ {
	ImGuiKeyboardCol_BoardBackground,		// Board background color
	ImGuiKeyboardCol_KeyBackground,			// Key background color
	ImGuiKeyboardCol_KeyBorder,				// Key border color
	ImGuiKeyboardCol_KeyFaceBorder,			// Key face border color
	ImGuiKeyboardCol_KeyFace,				// Key face fill color
	ImGuiKeyboardCol_KeyLabel,				// Key label text color
	ImGuiKeyboardCol_KeyPressed,			// Overlay color when key is pressed
	ImGuiKeyboardCol_KeyHighlighted,		// Overlay color when key is highlighted
	ImGuiKeyboardCol_KeyPressedHighlighted, // Overlay color when key is both pressed and highlighted
	ImGuiKeyboardCol_KeyRecorded,			// Overlay color when key is recorded (for keybinding selection)

	ImGuiKeyboardCol_COUNT
};
typedef int ImGuiKeyboardCol;

// Style struct for keyboard appearance
struct ImGuiKeyboardStyle {
	// Sizes (in pixels, before scaling)
	float KeyUnit;			 // Base key size unit (default: 34.0f)
	float SectionGap;		 // Gap between keyboard sections (default: 15.0f)
	float KeyBorderSize;	 // Key border thickness - defines visual gap between keys (default: 1.0f)
	float KeyRounding;		 // Key corner rounding (default: 3.0f)
	float KeyFaceRounding;	 // Key face corner rounding (default: 2.0f)
	float KeyFaceBorderSize; // Key face border thickness (default: 2.0f)
	ImVec2 KeyFaceOffset;	 // Offset of key face from key edge (default: 4.0f, 3.0f)
	ImVec2 KeyLabelOffset;	 // Offset of label from key edge (default: 6.0f, 4.0f)
	float BoardPadding;		 // Padding around keyboard (default: 5.0f)
	float BoardRounding;	 // Board corner rounding (default: 5.0f)

	// Colors
	ImVec4 Colors[ImGuiKeyboardCol_COUNT];

	ImGuiKeyboardStyle();
};

ImGuiKeyboardStyle &GetStyle();
void Highlight(ImGuiKey key, bool highlight);
void ClearHighlights();
void ClearRecorded();
const ImVector<ImGuiKey> &GetRecordedKeys();
void Keyboard(ImGuiKeyboardLayout layout, ImGuiKeyboardFlags flags = 0);
void KeyboardDemo();

} // namespace ImKeyboard
