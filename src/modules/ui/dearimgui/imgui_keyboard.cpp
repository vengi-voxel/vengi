#include "imgui_keyboard.h"
#include "imgui_internal.h"

namespace ImKeyboard {

#ifndef IMGUI_DISABLE

ImGuiKeyboardStyle::ImGuiKeyboardStyle() {
	KeyUnit = 34.0f;
	SectionGap = 15.0f;
	KeyBorderSize = 1.0f;
	KeyRounding = 3.0f;
	KeyFaceRounding = 2.0f;
	KeyFaceBorderSize = 2.0f;
	KeyFaceOffset = ImVec2(4.0f, 3.0f);
	KeyLabelOffset = ImVec2(6.0f, 4.0f);
	BoardPadding = 5.0f;
	BoardRounding = 5.0f;

	Colors[ImGuiKeyboardCol_BoardBackground] = ImVec4(0.2f, 0.2f, 0.2f, 0.0f);		 // Dark gray
	Colors[ImGuiKeyboardCol_KeyBackground] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);		 // Light gray
	Colors[ImGuiKeyboardCol_KeyBorder] = ImVec4(0.094f, 0.094f, 0.094f, 1.0f);		 // Dark gray
	Colors[ImGuiKeyboardCol_KeyFaceBorder] = ImVec4(0.757f, 0.757f, 0.757f, 1.0f);	 // Medium gray
	Colors[ImGuiKeyboardCol_KeyFace] = ImVec4(0.988f, 0.988f, 0.988f, 1.0f);		 // Near white
	Colors[ImGuiKeyboardCol_KeyLabel] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);			 // Dark text
	Colors[ImGuiKeyboardCol_KeyPressed] = ImVec4(1.0f, 0.0f, 0.0f, 0.5f);			 // Red
	Colors[ImGuiKeyboardCol_KeyHighlighted] = ImVec4(0.0f, 1.0f, 0.0f, 0.5f);		 // Green
	Colors[ImGuiKeyboardCol_KeyPressedHighlighted] = ImVec4(1.0f, 1.0f, 0.0f, 0.5f); // Yellow
	Colors[ImGuiKeyboardCol_KeyRecorded] = ImVec4(0.0f, 0.5f, 1.0f, 0.5f);			 // Blue (for keybinding selection)
}

struct KeyLayoutData {
	float X, Y;			 // Position in key units (not pixels)
	float Width, Height; // Size in key units (1.0 = standard key)
	const char *Label;
	const char *ShiftLabel; // Label when Shift is pressed (nullptr if same as Label)
	ImGuiKey Key;
};

struct KeyboardContext {
	ImVector<ImGuiKey> HighlightedKeys;
	ImVector<ImGuiKey> RecordedKeys;
	ImGuiKeyboardStyle Style;

	KeyboardContext() {
	}
};

static KeyboardContext *GetContext() {
	static KeyboardContext ctx;
	return &ctx;
}

ImGuiKeyboardStyle &GetStyle() {
	return GetContext()->Style;
}

static ImU32 GetColorU32(ImGuiKeyboardCol idx) {
	return ImGui::ColorConvertFloat4ToU32(GetContext()->Style.Colors[idx]);
}

static bool IsKeyHighlighted(ImGuiKey key) {
	KeyboardContext *ctx = GetContext();
	for (int i = 0; i < ctx->HighlightedKeys.Size; i++) {
		if (ctx->HighlightedKeys[i] == key) {
			return true;
		}
	}
	return false;
}

static bool IsKeyRecorded(ImGuiKey key) {
	KeyboardContext *ctx = GetContext();
	for (int i = 0; i < ctx->RecordedKeys.Size; i++) {
		if (ctx->RecordedKeys[i] == key) {
			return true;
		}
	}
	return false;
}

// Numeric keypad layout
static const KeyLayoutData numpad_keys[] = {
	// Row 0 - NumLock, /, *, -
	{0.0f, 0.0f, 1.0f, 1.0f, "Num", nullptr, ImGuiKey_NumLock},
	{1.0f, 0.0f, 1.0f, 1.0f, "/", nullptr, ImGuiKey_KeypadDivide},
	{2.0f, 0.0f, 1.0f, 1.0f, "*", nullptr, ImGuiKey_KeypadMultiply},
	{3.0f, 0.0f, 1.0f, 1.0f, "-", nullptr, ImGuiKey_KeypadSubtract},
	// Row 1 - 7, 8, 9, + (+ spans 2 rows)
	{0.0f, 1.0f, 1.0f, 1.0f, "7", nullptr, ImGuiKey_Keypad7},
	{1.0f, 1.0f, 1.0f, 1.0f, "8", nullptr, ImGuiKey_Keypad8},
	{2.0f, 1.0f, 1.0f, 1.0f, "9", nullptr, ImGuiKey_Keypad9},
	{3.0f, 1.0f, 1.0f, 2.0f, "+", nullptr, ImGuiKey_KeypadAdd},
	// Row 2 - 4, 5, 6
	{0.0f, 2.0f, 1.0f, 1.0f, "4", nullptr, ImGuiKey_Keypad4},
	{1.0f, 2.0f, 1.0f, 1.0f, "5", nullptr, ImGuiKey_Keypad5},
	{2.0f, 2.0f, 1.0f, 1.0f, "6", nullptr, ImGuiKey_Keypad6},
	// Row 3 - 1, 2, 3, Enter (Enter spans 2 rows)
	{0.0f, 3.0f, 1.0f, 1.0f, "1", nullptr, ImGuiKey_Keypad1},
	{1.0f, 3.0f, 1.0f, 1.0f, "2", nullptr, ImGuiKey_Keypad2},
	{2.0f, 3.0f, 1.0f, 1.0f, "3", nullptr, ImGuiKey_Keypad3},
	{3.0f, 3.0f, 1.0f, 2.0f, "Ent", nullptr, ImGuiKey_KeypadEnter},
	// Row 4 - 0 (spans 2 columns), .
	{0.0f, 4.0f, 2.0f, 1.0f, "0", nullptr, ImGuiKey_Keypad0},
	{2.0f, 4.0f, 1.0f, 1.0f, ".", nullptr, ImGuiKey_KeypadDecimal},
};

// Function key row (F1-F12 + Esc)
static const KeyLayoutData function_row_keys[] = {
	{0.0f, 0.0f, 1.0f, 1.0f, "Esc", nullptr, ImGuiKey_Escape},
	// Gap
	{2.0f, 0.0f, 1.0f, 1.0f, "F1", nullptr, ImGuiKey_F1},
	{3.0f, 0.0f, 1.0f, 1.0f, "F2", nullptr, ImGuiKey_F2},
	{4.0f, 0.0f, 1.0f, 1.0f, "F3", nullptr, ImGuiKey_F3},
	{5.0f, 0.0f, 1.0f, 1.0f, "F4", nullptr, ImGuiKey_F4},
	// Gap
	{6.5f, 0.0f, 1.0f, 1.0f, "F5", nullptr, ImGuiKey_F5},
	{7.5f, 0.0f, 1.0f, 1.0f, "F6", nullptr, ImGuiKey_F6},
	{8.5f, 0.0f, 1.0f, 1.0f, "F7", nullptr, ImGuiKey_F7},
	{9.5f, 0.0f, 1.0f, 1.0f, "F8", nullptr, ImGuiKey_F8},
	// Gap
	{11.0f, 0.0f, 1.0f, 1.0f, "F9", nullptr, ImGuiKey_F9},
	{12.0f, 0.0f, 1.0f, 1.0f, "F10", nullptr, ImGuiKey_F10},
	{13.0f, 0.0f, 1.0f, 1.0f, "F11", nullptr, ImGuiKey_F11},
	{14.0f, 0.0f, 1.0f, 1.0f, "F12", nullptr, ImGuiKey_F12},
};

// Print, Scroll, Pause - rendered separately to align with nav cluster using section_gap
static const KeyLayoutData function_row_nav_keys[] = {
	{0.0f, 0.0f, 1.0f, 1.0f, "Prt", nullptr, ImGuiKey_PrintScreen},
	{1.0f, 0.0f, 1.0f, 1.0f, "Scr", nullptr, ImGuiKey_ScrollLock},
	{2.0f, 0.0f, 1.0f, 1.0f, "Pse", nullptr, ImGuiKey_Pause},
};

// Apple layout: F13, F14, F15 instead of Print Screen, Scroll Lock, Pause
static const KeyLayoutData apple_function_row_nav_keys[] = {
	{0.0f, 0.0f, 1.0f, 1.0f, "F13", nullptr, ImGuiKey_PrintScreen},
	{1.0f, 0.0f, 1.0f, 1.0f, "F14", nullptr, ImGuiKey_ScrollLock},
	{2.0f, 0.0f, 1.0f, 1.0f, "F15", nullptr, ImGuiKey_Pause},
};

// Navigation cluster (Insert, Delete, Home, End, PageUp, PageDown, Arrows)
static const KeyLayoutData nav_cluster_keys[] = {
	// Row 0 - Insert, Home, PageUp
	{0.0f, 0.0f, 1.0f, 1.0f, "Ins", nullptr, ImGuiKey_Insert},
	{1.0f, 0.0f, 1.0f, 1.0f, "Hm", nullptr, ImGuiKey_Home},
	{2.0f, 0.0f, 1.0f, 1.0f, "PgU", nullptr, ImGuiKey_PageUp},
	// Row 1 - Delete, End, PageDown
	{0.0f, 1.0f, 1.0f, 1.0f, "Del", nullptr, ImGuiKey_Delete},
	{1.0f, 1.0f, 1.0f, 1.0f, "End", nullptr, ImGuiKey_End},
	{2.0f, 1.0f, 1.0f, 1.0f, "PgD", nullptr, ImGuiKey_PageDown},
	// Row 3 - Arrow Up (centered) - aligned with bottom modifier row
	{1.0f, 3.0f, 1.0f, 1.0f, "^", nullptr, ImGuiKey_UpArrow},
	// Row 4 - Arrow Left, Down, Right
	{0.0f, 4.0f, 1.0f, 1.0f, "<", nullptr, ImGuiKey_LeftArrow},
	{1.0f, 4.0f, 1.0f, 1.0f, "v", nullptr, ImGuiKey_DownArrow},
	{2.0f, 4.0f, 1.0f, 1.0f, ">", nullptr, ImGuiKey_RightArrow},
};

// Main keyboard - Number row (US layout shift symbols)
static const KeyLayoutData number_row_keys[] = {
	{0.0f, 0.0f, 1.0f, 1.0f, "`", "~", ImGuiKey_GraveAccent},
	{1.0f, 0.0f, 1.0f, 1.0f, "1", "!", ImGuiKey_1},
	{2.0f, 0.0f, 1.0f, 1.0f, "2", "@", ImGuiKey_2},
	{3.0f, 0.0f, 1.0f, 1.0f, "3", "#", ImGuiKey_3},
	{4.0f, 0.0f, 1.0f, 1.0f, "4", "$", ImGuiKey_4},
	{5.0f, 0.0f, 1.0f, 1.0f, "5", "%", ImGuiKey_5},
	{6.0f, 0.0f, 1.0f, 1.0f, "6", "^", ImGuiKey_6},
	{7.0f, 0.0f, 1.0f, 1.0f, "7", "&", ImGuiKey_7},
	{8.0f, 0.0f, 1.0f, 1.0f, "8", "*", ImGuiKey_8},
	{9.0f, 0.0f, 1.0f, 1.0f, "9", "(", ImGuiKey_9},
	{10.0f, 0.0f, 1.0f, 1.0f, "0", ")", ImGuiKey_0},
	{11.0f, 0.0f, 1.0f, 1.0f, "-", "_", ImGuiKey_Minus},
	{12.0f, 0.0f, 1.0f, 1.0f, "=", "+", ImGuiKey_Equal},
	{13.0f, 0.0f, 2.0f, 1.0f, "Back", nullptr, ImGuiKey_Backspace},
};

// German number row (QWERTZ layout shift symbols)
static const KeyLayoutData number_row_qwertz_keys[] = {
	{0.0f, 0.0f, 1.0f, 1.0f, "^", nullptr, ImGuiKey_GraveAccent},
	{1.0f, 0.0f, 1.0f, 1.0f, "1", "!", ImGuiKey_1},
	{2.0f, 0.0f, 1.0f, 1.0f, "2", "\"", ImGuiKey_2},
	{3.0f, 0.0f, 1.0f, 1.0f, "3", "\xc2\xa7", ImGuiKey_3},
	{4.0f, 0.0f, 1.0f, 1.0f, "4", "$", ImGuiKey_4},
	{5.0f, 0.0f, 1.0f, 1.0f, "5", "%", ImGuiKey_5},
	{6.0f, 0.0f, 1.0f, 1.0f, "6", "&", ImGuiKey_6},
	{7.0f, 0.0f, 1.0f, 1.0f, "7", "/", ImGuiKey_7},
	{8.0f, 0.0f, 1.0f, 1.0f, "8", "(", ImGuiKey_8},
	{9.0f, 0.0f, 1.0f, 1.0f, "9", ")", ImGuiKey_9},
	{10.0f, 0.0f, 1.0f, 1.0f, "0", "=", ImGuiKey_0},
	{11.0f, 0.0f, 1.0f, 1.0f, "\xc3\x9f", "?", ImGuiKey_Minus},
	{12.0f, 0.0f, 1.0f, 1.0f, "'", "`", ImGuiKey_Equal},
	{13.0f, 0.0f, 2.0f, 1.0f, "Back", nullptr, ImGuiKey_Backspace},
};

// French number row (AZERTY layout - numbers require shift)
static const KeyLayoutData number_row_azerty_keys[] = {
	{0.0f, 0.0f, 1.0f, 1.0f, "2", nullptr, ImGuiKey_GraveAccent},
	{1.0f, 0.0f, 1.0f, 1.0f, "&", "1", ImGuiKey_1},
	{2.0f, 0.0f, 1.0f, 1.0f, "\xc3\xa9", "2", ImGuiKey_2},
	{3.0f, 0.0f, 1.0f, 1.0f, "\"", "3", ImGuiKey_3},
	{4.0f, 0.0f, 1.0f, 1.0f, "'", "4", ImGuiKey_4},
	{5.0f, 0.0f, 1.0f, 1.0f, "(", "5", ImGuiKey_5},
	{6.0f, 0.0f, 1.0f, 1.0f, "-", "6", ImGuiKey_6},
	{7.0f, 0.0f, 1.0f, 1.0f, "\xc3\xa8", "7", ImGuiKey_7},
	{8.0f, 0.0f, 1.0f, 1.0f, "_", "8", ImGuiKey_8},
	{9.0f, 0.0f, 1.0f, 1.0f, "\xc3\xa7", "9", ImGuiKey_9},
	{10.0f, 0.0f, 1.0f, 1.0f, "\xc3\xa0", "0", ImGuiKey_0},
	{11.0f, 0.0f, 1.0f, 1.0f, ")", nullptr, ImGuiKey_Minus},
	{12.0f, 0.0f, 1.0f, 1.0f, "=", "+", ImGuiKey_Equal},
	{13.0f, 0.0f, 2.0f, 1.0f, "Back", nullptr, ImGuiKey_Backspace},
};

// QWERTY letter rows
static const KeyLayoutData qwerty_row1_keys[] = {
	{0.0f, 0.0f, 1.5f, 1.0f, "Tab", nullptr, ImGuiKey_Tab},
	{1.5f, 0.0f, 1.0f, 1.0f, "Q", nullptr, ImGuiKey_Q},
	{2.5f, 0.0f, 1.0f, 1.0f, "W", nullptr, ImGuiKey_W},
	{3.5f, 0.0f, 1.0f, 1.0f, "E", nullptr, ImGuiKey_E},
	{4.5f, 0.0f, 1.0f, 1.0f, "R", nullptr, ImGuiKey_R},
	{5.5f, 0.0f, 1.0f, 1.0f, "T", nullptr, ImGuiKey_T},
	{6.5f, 0.0f, 1.0f, 1.0f, "Y", nullptr, ImGuiKey_Y},
	{7.5f, 0.0f, 1.0f, 1.0f, "U", nullptr, ImGuiKey_U},
	{8.5f, 0.0f, 1.0f, 1.0f, "I", nullptr, ImGuiKey_I},
	{9.5f, 0.0f, 1.0f, 1.0f, "O", nullptr, ImGuiKey_O},
	{10.5f, 0.0f, 1.0f, 1.0f, "P", nullptr, ImGuiKey_P},
	{11.5f, 0.0f, 1.0f, 1.0f, "[", "{", ImGuiKey_LeftBracket},
	{12.5f, 0.0f, 1.0f, 1.0f, "]", "}", ImGuiKey_RightBracket},
	{13.5f, 0.0f, 1.5f, 1.0f, "\\", "|", ImGuiKey_Backslash},
};

static const KeyLayoutData qwerty_row2_keys[] = {
	{0.0f, 0.0f, 1.75f, 1.0f, "Caps", nullptr, ImGuiKey_CapsLock},
	{1.75f, 0.0f, 1.0f, 1.0f, "A", nullptr, ImGuiKey_A},
	{2.75f, 0.0f, 1.0f, 1.0f, "S", nullptr, ImGuiKey_S},
	{3.75f, 0.0f, 1.0f, 1.0f, "D", nullptr, ImGuiKey_D},
	{4.75f, 0.0f, 1.0f, 1.0f, "F", nullptr, ImGuiKey_F},
	{5.75f, 0.0f, 1.0f, 1.0f, "G", nullptr, ImGuiKey_G},
	{6.75f, 0.0f, 1.0f, 1.0f, "H", nullptr, ImGuiKey_H},
	{7.75f, 0.0f, 1.0f, 1.0f, "J", nullptr, ImGuiKey_J},
	{8.75f, 0.0f, 1.0f, 1.0f, "K", nullptr, ImGuiKey_K},
	{9.75f, 0.0f, 1.0f, 1.0f, "L", nullptr, ImGuiKey_L},
	{10.75f, 0.0f, 1.0f, 1.0f, ";", ":", ImGuiKey_Semicolon},
	{11.75f, 0.0f, 1.0f, 1.0f, "'", "\"", ImGuiKey_Apostrophe},
	{12.75f, 0.0f, 2.25f, 1.0f, "Enter", nullptr, ImGuiKey_Enter},
};

static const KeyLayoutData qwerty_row3_keys[] = {
	{0.0f, 0.0f, 2.25f, 1.0f, "Shift", nullptr, ImGuiKey_LeftShift},
	{2.25f, 0.0f, 1.0f, 1.0f, "Z", nullptr, ImGuiKey_Z},
	{3.25f, 0.0f, 1.0f, 1.0f, "X", nullptr, ImGuiKey_X},
	{4.25f, 0.0f, 1.0f, 1.0f, "C", nullptr, ImGuiKey_C},
	{5.25f, 0.0f, 1.0f, 1.0f, "V", nullptr, ImGuiKey_V},
	{6.25f, 0.0f, 1.0f, 1.0f, "B", nullptr, ImGuiKey_B},
	{7.25f, 0.0f, 1.0f, 1.0f, "N", nullptr, ImGuiKey_N},
	{8.25f, 0.0f, 1.0f, 1.0f, "M", nullptr, ImGuiKey_M},
	{9.25f, 0.0f, 1.0f, 1.0f, ",", "<", ImGuiKey_Comma},
	{10.25f, 0.0f, 1.0f, 1.0f, ".", ">", ImGuiKey_Period},
	{11.25f, 0.0f, 1.0f, 1.0f, "/", "?", ImGuiKey_Slash},
	{12.25f, 0.0f, 2.75f, 1.0f, "Shift", nullptr, ImGuiKey_RightShift},
};

// QWERTZ letter rows (German layout - Y and Z swapped)
static const KeyLayoutData qwertz_row1_keys[] = {
	{0.0f, 0.0f, 1.5f, 1.0f, "Tab", nullptr, ImGuiKey_Tab},
	{1.5f, 0.0f, 1.0f, 1.0f, "Q", nullptr, ImGuiKey_Q},
	{2.5f, 0.0f, 1.0f, 1.0f, "W", nullptr, ImGuiKey_W},
	{3.5f, 0.0f, 1.0f, 1.0f, "E", nullptr, ImGuiKey_E},
	{4.5f, 0.0f, 1.0f, 1.0f, "R", nullptr, ImGuiKey_R},
	{5.5f, 0.0f, 1.0f, 1.0f, "T", nullptr, ImGuiKey_T},
	{6.5f, 0.0f, 1.0f, 1.0f, "Z", nullptr, ImGuiKey_Z},
	{7.5f, 0.0f, 1.0f, 1.0f, "U", nullptr, ImGuiKey_U},
	{8.5f, 0.0f, 1.0f, 1.0f, "I", nullptr, ImGuiKey_I},
	{9.5f, 0.0f, 1.0f, 1.0f, "O", nullptr, ImGuiKey_O},
	{10.5f, 0.0f, 1.0f, 1.0f, "P", nullptr, ImGuiKey_P},
	{11.5f, 0.0f, 1.0f, 1.0f, "\xc3\x9c", nullptr, ImGuiKey_LeftBracket},
	{12.5f, 0.0f, 1.0f, 1.0f, "+", "*", ImGuiKey_RightBracket},
	{13.5f, 0.0f, 1.5f, 1.0f, "#", "'", ImGuiKey_Backslash},
};

static const KeyLayoutData qwertz_row2_keys[] = {
	{0.0f, 0.0f, 1.75f, 1.0f, "Caps", nullptr, ImGuiKey_CapsLock},
	{1.75f, 0.0f, 1.0f, 1.0f, "A", nullptr, ImGuiKey_A},
	{2.75f, 0.0f, 1.0f, 1.0f, "S", nullptr, ImGuiKey_S},
	{3.75f, 0.0f, 1.0f, 1.0f, "D", nullptr, ImGuiKey_D},
	{4.75f, 0.0f, 1.0f, 1.0f, "F", nullptr, ImGuiKey_F},
	{5.75f, 0.0f, 1.0f, 1.0f, "G", nullptr, ImGuiKey_G},
	{6.75f, 0.0f, 1.0f, 1.0f, "H", nullptr, ImGuiKey_H},
	{7.75f, 0.0f, 1.0f, 1.0f, "J", nullptr, ImGuiKey_J},
	{8.75f, 0.0f, 1.0f, 1.0f, "K", nullptr, ImGuiKey_K},
	{9.75f, 0.0f, 1.0f, 1.0f, "L", nullptr, ImGuiKey_L},
	{10.75f, 0.0f, 1.0f, 1.0f, "\xc3\x96", nullptr, ImGuiKey_Semicolon},
	{11.75f, 0.0f, 1.0f, 1.0f, "\xc3\x84", nullptr, ImGuiKey_Apostrophe},
	{12.75f, 0.0f, 2.25f, 1.0f, "Enter", nullptr, ImGuiKey_Enter},
};

static const KeyLayoutData qwertz_row3_keys[] = {
	{0.0f, 0.0f, 2.25f, 1.0f, "Shift", nullptr, ImGuiKey_LeftShift},
	{2.25f, 0.0f, 1.0f, 1.0f, "Y", nullptr, ImGuiKey_Y},
	{3.25f, 0.0f, 1.0f, 1.0f, "X", nullptr, ImGuiKey_X},
	{4.25f, 0.0f, 1.0f, 1.0f, "C", nullptr, ImGuiKey_C},
	{5.25f, 0.0f, 1.0f, 1.0f, "V", nullptr, ImGuiKey_V},
	{6.25f, 0.0f, 1.0f, 1.0f, "B", nullptr, ImGuiKey_B},
	{7.25f, 0.0f, 1.0f, 1.0f, "N", nullptr, ImGuiKey_N},
	{8.25f, 0.0f, 1.0f, 1.0f, "M", nullptr, ImGuiKey_M},
	{9.25f, 0.0f, 1.0f, 1.0f, ",", ";", ImGuiKey_Comma},
	{10.25f, 0.0f, 1.0f, 1.0f, ".", ":", ImGuiKey_Period},
	{11.25f, 0.0f, 1.0f, 1.0f, "-", "_", ImGuiKey_Slash},
	{12.25f, 0.0f, 2.75f, 1.0f, "Shift", nullptr, ImGuiKey_RightShift},
};

// AZERTY letter rows (French layout)
static const KeyLayoutData azerty_row1_keys[] = {
	{0.0f, 0.0f, 1.5f, 1.0f, "Tab", nullptr, ImGuiKey_Tab},
	{1.5f, 0.0f, 1.0f, 1.0f, "A", nullptr, ImGuiKey_A},
	{2.5f, 0.0f, 1.0f, 1.0f, "Z", nullptr, ImGuiKey_Z},
	{3.5f, 0.0f, 1.0f, 1.0f, "E", nullptr, ImGuiKey_E},
	{4.5f, 0.0f, 1.0f, 1.0f, "R", nullptr, ImGuiKey_R},
	{5.5f, 0.0f, 1.0f, 1.0f, "T", nullptr, ImGuiKey_T},
	{6.5f, 0.0f, 1.0f, 1.0f, "Y", nullptr, ImGuiKey_Y},
	{7.5f, 0.0f, 1.0f, 1.0f, "U", nullptr, ImGuiKey_U},
	{8.5f, 0.0f, 1.0f, 1.0f, "I", nullptr, ImGuiKey_I},
	{9.5f, 0.0f, 1.0f, 1.0f, "O", nullptr, ImGuiKey_O},
	{10.5f, 0.0f, 1.0f, 1.0f, "P", nullptr, ImGuiKey_P},
	{11.5f, 0.0f, 1.0f, 1.0f, "^", nullptr, ImGuiKey_LeftBracket},
	{12.5f, 0.0f, 1.0f, 1.0f, "$", nullptr, ImGuiKey_RightBracket},
	{13.5f, 0.0f, 1.5f, 1.0f, "*", nullptr, ImGuiKey_Backslash},
};

static const KeyLayoutData azerty_row2_keys[] = {
	{0.0f, 0.0f, 1.75f, 1.0f, "Caps", nullptr, ImGuiKey_CapsLock},
	{1.75f, 0.0f, 1.0f, 1.0f, "Q", nullptr, ImGuiKey_Q},
	{2.75f, 0.0f, 1.0f, 1.0f, "S", nullptr, ImGuiKey_S},
	{3.75f, 0.0f, 1.0f, 1.0f, "D", nullptr, ImGuiKey_D},
	{4.75f, 0.0f, 1.0f, 1.0f, "F", nullptr, ImGuiKey_F},
	{5.75f, 0.0f, 1.0f, 1.0f, "G", nullptr, ImGuiKey_G},
	{6.75f, 0.0f, 1.0f, 1.0f, "H", nullptr, ImGuiKey_H},
	{7.75f, 0.0f, 1.0f, 1.0f, "J", nullptr, ImGuiKey_J},
	{8.75f, 0.0f, 1.0f, 1.0f, "K", nullptr, ImGuiKey_K},
	{9.75f, 0.0f, 1.0f, 1.0f, "L", nullptr, ImGuiKey_L},
	{10.75f, 0.0f, 1.0f, 1.0f, "M", nullptr, ImGuiKey_M},
	{11.75f, 0.0f, 1.0f, 1.0f, "\xc3\xb9", "%", ImGuiKey_Apostrophe},
	{12.75f, 0.0f, 2.25f, 1.0f, "Enter", nullptr, ImGuiKey_Enter},
};

static const KeyLayoutData azerty_row3_keys[] = {
	{0.0f, 0.0f, 2.25f, 1.0f, "Shift", nullptr, ImGuiKey_LeftShift},
	{2.25f, 0.0f, 1.0f, 1.0f, "W", nullptr, ImGuiKey_W},
	{3.25f, 0.0f, 1.0f, 1.0f, "X", nullptr, ImGuiKey_X},
	{4.25f, 0.0f, 1.0f, 1.0f, "C", nullptr, ImGuiKey_C},
	{5.25f, 0.0f, 1.0f, 1.0f, "V", nullptr, ImGuiKey_V},
	{6.25f, 0.0f, 1.0f, 1.0f, "B", nullptr, ImGuiKey_B},
	{7.25f, 0.0f, 1.0f, 1.0f, "N", nullptr, ImGuiKey_N},
	{8.25f, 0.0f, 1.0f, 1.0f, ",", "?", ImGuiKey_Comma},
	{9.25f, 0.0f, 1.0f, 1.0f, ";", ".", ImGuiKey_Semicolon},
	{10.25f, 0.0f, 1.0f, 1.0f, ":", "/", ImGuiKey_Period},
	{11.25f, 0.0f, 1.0f, 1.0f, "!", nullptr, ImGuiKey_Slash},
	{12.25f, 0.0f, 2.75f, 1.0f, "Shift", nullptr, ImGuiKey_RightShift},
};

// Colemak letter rows
static const KeyLayoutData colemak_row1_keys[] = {
	{0.0f, 0.0f, 1.5f, 1.0f, "Tab", nullptr, ImGuiKey_Tab},
	{1.5f, 0.0f, 1.0f, 1.0f, "Q", nullptr, ImGuiKey_Q},
	{2.5f, 0.0f, 1.0f, 1.0f, "W", nullptr, ImGuiKey_W},
	{3.5f, 0.0f, 1.0f, 1.0f, "F", nullptr, ImGuiKey_F},
	{4.5f, 0.0f, 1.0f, 1.0f, "P", nullptr, ImGuiKey_P},
	{5.5f, 0.0f, 1.0f, 1.0f, "G", nullptr, ImGuiKey_G},
	{6.5f, 0.0f, 1.0f, 1.0f, "J", nullptr, ImGuiKey_J},
	{7.5f, 0.0f, 1.0f, 1.0f, "L", nullptr, ImGuiKey_L},
	{8.5f, 0.0f, 1.0f, 1.0f, "U", nullptr, ImGuiKey_U},
	{9.5f, 0.0f, 1.0f, 1.0f, "Y", nullptr, ImGuiKey_Y},
	{10.5f, 0.0f, 1.0f, 1.0f, ";", ":", ImGuiKey_Semicolon},
	{11.5f, 0.0f, 1.0f, 1.0f, "[", "{", ImGuiKey_LeftBracket},
	{12.5f, 0.0f, 1.0f, 1.0f, "]", "}", ImGuiKey_RightBracket},
	{13.5f, 0.0f, 1.5f, 1.0f, "\\", "|", ImGuiKey_Backslash},
};

static const KeyLayoutData colemak_row2_keys[] = {
	{0.0f, 0.0f, 1.75f, 1.0f, "Bksp", nullptr, ImGuiKey_Backspace},
	{1.75f, 0.0f, 1.0f, 1.0f, "A", nullptr, ImGuiKey_A},
	{2.75f, 0.0f, 1.0f, 1.0f, "R", nullptr, ImGuiKey_R},
	{3.75f, 0.0f, 1.0f, 1.0f, "S", nullptr, ImGuiKey_S},
	{4.75f, 0.0f, 1.0f, 1.0f, "T", nullptr, ImGuiKey_T},
	{5.75f, 0.0f, 1.0f, 1.0f, "D", nullptr, ImGuiKey_D},
	{6.75f, 0.0f, 1.0f, 1.0f, "H", nullptr, ImGuiKey_H},
	{7.75f, 0.0f, 1.0f, 1.0f, "N", nullptr, ImGuiKey_N},
	{8.75f, 0.0f, 1.0f, 1.0f, "E", nullptr, ImGuiKey_E},
	{9.75f, 0.0f, 1.0f, 1.0f, "I", nullptr, ImGuiKey_I},
	{10.75f, 0.0f, 1.0f, 1.0f, "O", nullptr, ImGuiKey_O},
	{11.75f, 0.0f, 1.0f, 1.0f, "'", "\"", ImGuiKey_Apostrophe},
	{12.75f, 0.0f, 2.25f, 1.0f, "Enter", nullptr, ImGuiKey_Enter},
};

static const KeyLayoutData colemak_row3_keys[] = {
	{0.0f, 0.0f, 2.25f, 1.0f, "Shift", nullptr, ImGuiKey_LeftShift},
	{2.25f, 0.0f, 1.0f, 1.0f, "Z", nullptr, ImGuiKey_Z},
	{3.25f, 0.0f, 1.0f, 1.0f, "X", nullptr, ImGuiKey_X},
	{4.25f, 0.0f, 1.0f, 1.0f, "C", nullptr, ImGuiKey_C},
	{5.25f, 0.0f, 1.0f, 1.0f, "V", nullptr, ImGuiKey_V},
	{6.25f, 0.0f, 1.0f, 1.0f, "B", nullptr, ImGuiKey_B},
	{7.25f, 0.0f, 1.0f, 1.0f, "K", nullptr, ImGuiKey_K},
	{8.25f, 0.0f, 1.0f, 1.0f, "M", nullptr, ImGuiKey_M},
	{9.25f, 0.0f, 1.0f, 1.0f, ",", "<", ImGuiKey_Comma},
	{10.25f, 0.0f, 1.0f, 1.0f, ".", ">", ImGuiKey_Period},
	{11.25f, 0.0f, 1.0f, 1.0f, "/", "?", ImGuiKey_Slash},
	{12.25f, 0.0f, 2.75f, 1.0f, "Shift", nullptr, ImGuiKey_RightShift},
};

// Dvorak letter rows
static const KeyLayoutData dvorak_row1_keys[] = {
	{0.0f, 0.0f, 1.5f, 1.0f, "Tab", nullptr, ImGuiKey_Tab}, {1.5f, 0.0f, 1.0f, 1.0f, "'", "\"", ImGuiKey_Apostrophe},
	{2.5f, 0.0f, 1.0f, 1.0f, ",", "<", ImGuiKey_Comma},		{3.5f, 0.0f, 1.0f, 1.0f, ".", ">", ImGuiKey_Period},
	{4.5f, 0.0f, 1.0f, 1.0f, "P", nullptr, ImGuiKey_P},		{5.5f, 0.0f, 1.0f, 1.0f, "Y", nullptr, ImGuiKey_Y},
	{6.5f, 0.0f, 1.0f, 1.0f, "F", nullptr, ImGuiKey_F},		{7.5f, 0.0f, 1.0f, 1.0f, "G", nullptr, ImGuiKey_G},
	{8.5f, 0.0f, 1.0f, 1.0f, "C", nullptr, ImGuiKey_C},		{9.5f, 0.0f, 1.0f, 1.0f, "R", nullptr, ImGuiKey_R},
	{10.5f, 0.0f, 1.0f, 1.0f, "L", nullptr, ImGuiKey_L},	{11.5f, 0.0f, 1.0f, 1.0f, "/", "?", ImGuiKey_Slash},
	{12.5f, 0.0f, 1.0f, 1.0f, "=", "+", ImGuiKey_Equal},	{13.5f, 0.0f, 1.5f, 1.0f, "\\", "|", ImGuiKey_Backslash},
};

static const KeyLayoutData dvorak_row2_keys[] = {
	{0.0f, 0.0f, 1.75f, 1.0f, "Caps", nullptr, ImGuiKey_CapsLock}, {1.75f, 0.0f, 1.0f, 1.0f, "A", nullptr, ImGuiKey_A},
	{2.75f, 0.0f, 1.0f, 1.0f, "O", nullptr, ImGuiKey_O},		   {3.75f, 0.0f, 1.0f, 1.0f, "E", nullptr, ImGuiKey_E},
	{4.75f, 0.0f, 1.0f, 1.0f, "U", nullptr, ImGuiKey_U},		   {5.75f, 0.0f, 1.0f, 1.0f, "I", nullptr, ImGuiKey_I},
	{6.75f, 0.0f, 1.0f, 1.0f, "D", nullptr, ImGuiKey_D},		   {7.75f, 0.0f, 1.0f, 1.0f, "H", nullptr, ImGuiKey_H},
	{8.75f, 0.0f, 1.0f, 1.0f, "T", nullptr, ImGuiKey_T},		   {9.75f, 0.0f, 1.0f, 1.0f, "N", nullptr, ImGuiKey_N},
	{10.75f, 0.0f, 1.0f, 1.0f, "S", nullptr, ImGuiKey_S},		   {11.75f, 0.0f, 1.0f, 1.0f, "-", "_", ImGuiKey_Minus},
	{12.75f, 0.0f, 2.25f, 1.0f, "Enter", nullptr, ImGuiKey_Enter},
};

static const KeyLayoutData dvorak_row3_keys[] = {
	{0.0f, 0.0f, 2.25f, 1.0f, "Shift", nullptr, ImGuiKey_LeftShift},
	{2.25f, 0.0f, 1.0f, 1.0f, ";", ":", ImGuiKey_Semicolon},
	{3.25f, 0.0f, 1.0f, 1.0f, "Q", nullptr, ImGuiKey_Q},
	{4.25f, 0.0f, 1.0f, 1.0f, "J", nullptr, ImGuiKey_J},
	{5.25f, 0.0f, 1.0f, 1.0f, "K", nullptr, ImGuiKey_K},
	{6.25f, 0.0f, 1.0f, 1.0f, "X", nullptr, ImGuiKey_X},
	{7.25f, 0.0f, 1.0f, 1.0f, "B", nullptr, ImGuiKey_B},
	{8.25f, 0.0f, 1.0f, 1.0f, "M", nullptr, ImGuiKey_M},
	{9.25f, 0.0f, 1.0f, 1.0f, "W", nullptr, ImGuiKey_W},
	{10.25f, 0.0f, 1.0f, 1.0f, "V", nullptr, ImGuiKey_V},
	{11.25f, 0.0f, 1.0f, 1.0f, "Z", nullptr, ImGuiKey_Z},
	{12.25f, 0.0f, 2.75f, 1.0f, "Shift", nullptr, ImGuiKey_RightShift},
};

// ISO layout rows - ISO keyboards have:
// - L-shaped Enter key (spans row 1 and row 2)
// - Shorter left Shift with extra key next to it
// - No backslash key on row 1 (it's part of Enter or moved to row 3)

// ISO QWERTY row 1 - Enter key rendered separately as L-shape
static const KeyLayoutData qwerty_iso_row1_keys[] = {
	{0.0f, 0.0f, 1.5f, 1.0f, "Tab", nullptr, ImGuiKey_Tab},
	{1.5f, 0.0f, 1.0f, 1.0f, "Q", nullptr, ImGuiKey_Q},
	{2.5f, 0.0f, 1.0f, 1.0f, "W", nullptr, ImGuiKey_W},
	{3.5f, 0.0f, 1.0f, 1.0f, "E", nullptr, ImGuiKey_E},
	{4.5f, 0.0f, 1.0f, 1.0f, "R", nullptr, ImGuiKey_R},
	{5.5f, 0.0f, 1.0f, 1.0f, "T", nullptr, ImGuiKey_T},
	{6.5f, 0.0f, 1.0f, 1.0f, "Y", nullptr, ImGuiKey_Y},
	{7.5f, 0.0f, 1.0f, 1.0f, "U", nullptr, ImGuiKey_U},
	{8.5f, 0.0f, 1.0f, 1.0f, "I", nullptr, ImGuiKey_I},
	{9.5f, 0.0f, 1.0f, 1.0f, "O", nullptr, ImGuiKey_O},
	{10.5f, 0.0f, 1.0f, 1.0f, "P", nullptr, ImGuiKey_P},
	{11.5f, 0.0f, 1.0f, 1.0f, "[", "{", ImGuiKey_LeftBracket},
	{12.5f, 0.0f, 1.0f, 1.0f, "]", "}", ImGuiKey_RightBracket},
};

// ISO QWERTY row 2 - Enter key is rendered separately as L-shape polygon
static const KeyLayoutData qwerty_iso_row2_keys[] = {
	{0.0f, 0.0f, 1.75f, 1.0f, "Caps", nullptr, ImGuiKey_CapsLock},
	{1.75f, 0.0f, 1.0f, 1.0f, "A", nullptr, ImGuiKey_A},
	{2.75f, 0.0f, 1.0f, 1.0f, "S", nullptr, ImGuiKey_S},
	{3.75f, 0.0f, 1.0f, 1.0f, "D", nullptr, ImGuiKey_D},
	{4.75f, 0.0f, 1.0f, 1.0f, "F", nullptr, ImGuiKey_F},
	{5.75f, 0.0f, 1.0f, 1.0f, "G", nullptr, ImGuiKey_G},
	{6.75f, 0.0f, 1.0f, 1.0f, "H", nullptr, ImGuiKey_H},
	{7.75f, 0.0f, 1.0f, 1.0f, "J", nullptr, ImGuiKey_J},
	{8.75f, 0.0f, 1.0f, 1.0f, "K", nullptr, ImGuiKey_K},
	{9.75f, 0.0f, 1.0f, 1.0f, "L", nullptr, ImGuiKey_L},
	{10.75f, 0.0f, 1.0f, 1.0f, ";", ":", ImGuiKey_Semicolon},
	{11.75f, 0.0f, 1.0f, 1.0f, "'", "\"", ImGuiKey_Apostrophe},
	{12.75f, 0.0f, 1.0f, 1.0f, "#", "~", ImGuiKey_Backslash}, // ISO hash key (left of Enter)
};

// ISO QWERTY row 3 - shorter left Shift with extra key
static const KeyLayoutData qwerty_iso_row3_keys[] = {
	{0.0f, 0.0f, 1.25f, 1.0f, "Shift", nullptr, ImGuiKey_LeftShift},
	{1.25f, 0.0f, 1.0f, 1.0f, "\\", "|", ImGuiKey_Oem102}, // ISO extra key
	{2.25f, 0.0f, 1.0f, 1.0f, "Z", nullptr, ImGuiKey_Z},
	{3.25f, 0.0f, 1.0f, 1.0f, "X", nullptr, ImGuiKey_X},
	{4.25f, 0.0f, 1.0f, 1.0f, "C", nullptr, ImGuiKey_C},
	{5.25f, 0.0f, 1.0f, 1.0f, "V", nullptr, ImGuiKey_V},
	{6.25f, 0.0f, 1.0f, 1.0f, "B", nullptr, ImGuiKey_B},
	{7.25f, 0.0f, 1.0f, 1.0f, "N", nullptr, ImGuiKey_N},
	{8.25f, 0.0f, 1.0f, 1.0f, "M", nullptr, ImGuiKey_M},
	{9.25f, 0.0f, 1.0f, 1.0f, ",", "<", ImGuiKey_Comma},
	{10.25f, 0.0f, 1.0f, 1.0f, ".", ">", ImGuiKey_Period},
	{11.25f, 0.0f, 1.0f, 1.0f, "/", "?", ImGuiKey_Slash},
	{12.25f, 0.0f, 2.75f, 1.0f, "Shift", nullptr, ImGuiKey_RightShift},
};

// ISO QWERTZ (German) rows
static const KeyLayoutData qwertz_iso_row1_keys[] = {
	{0.0f, 0.0f, 1.5f, 1.0f, "Tab", nullptr, ImGuiKey_Tab},
	{1.5f, 0.0f, 1.0f, 1.0f, "Q", nullptr, ImGuiKey_Q},
	{2.5f, 0.0f, 1.0f, 1.0f, "W", nullptr, ImGuiKey_W},
	{3.5f, 0.0f, 1.0f, 1.0f, "E", nullptr, ImGuiKey_E},
	{4.5f, 0.0f, 1.0f, 1.0f, "R", nullptr, ImGuiKey_R},
	{5.5f, 0.0f, 1.0f, 1.0f, "T", nullptr, ImGuiKey_T},
	{6.5f, 0.0f, 1.0f, 1.0f, "Z", nullptr, ImGuiKey_Z},
	{7.5f, 0.0f, 1.0f, 1.0f, "U", nullptr, ImGuiKey_U},
	{8.5f, 0.0f, 1.0f, 1.0f, "I", nullptr, ImGuiKey_I},
	{9.5f, 0.0f, 1.0f, 1.0f, "O", nullptr, ImGuiKey_O},
	{10.5f, 0.0f, 1.0f, 1.0f, "P", nullptr, ImGuiKey_P},
	{11.5f, 0.0f, 1.0f, 1.0f, "\xc3\x9c", nullptr, ImGuiKey_LeftBracket},
	{12.5f, 0.0f, 1.0f, 1.0f, "+", "*", ImGuiKey_RightBracket},
};

static const KeyLayoutData qwertz_iso_row2_keys[] = {
	{0.0f, 0.0f, 1.75f, 1.0f, "Caps", nullptr, ImGuiKey_CapsLock},
	{1.75f, 0.0f, 1.0f, 1.0f, "A", nullptr, ImGuiKey_A},
	{2.75f, 0.0f, 1.0f, 1.0f, "S", nullptr, ImGuiKey_S},
	{3.75f, 0.0f, 1.0f, 1.0f, "D", nullptr, ImGuiKey_D},
	{4.75f, 0.0f, 1.0f, 1.0f, "F", nullptr, ImGuiKey_F},
	{5.75f, 0.0f, 1.0f, 1.0f, "G", nullptr, ImGuiKey_G},
	{6.75f, 0.0f, 1.0f, 1.0f, "H", nullptr, ImGuiKey_H},
	{7.75f, 0.0f, 1.0f, 1.0f, "J", nullptr, ImGuiKey_J},
	{8.75f, 0.0f, 1.0f, 1.0f, "K", nullptr, ImGuiKey_K},
	{9.75f, 0.0f, 1.0f, 1.0f, "L", nullptr, ImGuiKey_L},
	{10.75f, 0.0f, 1.0f, 1.0f, "\xc3\x96", nullptr, ImGuiKey_Semicolon},
	{11.75f, 0.0f, 1.0f, 1.0f, "\xc3\x84", nullptr, ImGuiKey_Apostrophe},
	{12.75f, 0.0f, 1.0f, 1.0f, "#", "'", ImGuiKey_Backslash},
};

static const KeyLayoutData qwertz_iso_row3_keys[] = {
	{0.0f, 0.0f, 1.25f, 1.0f, "Shift", nullptr, ImGuiKey_LeftShift},
	{1.25f, 0.0f, 1.0f, 1.0f, "<", ">", ImGuiKey_Oem102},
	{2.25f, 0.0f, 1.0f, 1.0f, "Y", nullptr, ImGuiKey_Y},
	{3.25f, 0.0f, 1.0f, 1.0f, "X", nullptr, ImGuiKey_X},
	{4.25f, 0.0f, 1.0f, 1.0f, "C", nullptr, ImGuiKey_C},
	{5.25f, 0.0f, 1.0f, 1.0f, "V", nullptr, ImGuiKey_V},
	{6.25f, 0.0f, 1.0f, 1.0f, "B", nullptr, ImGuiKey_B},
	{7.25f, 0.0f, 1.0f, 1.0f, "N", nullptr, ImGuiKey_N},
	{8.25f, 0.0f, 1.0f, 1.0f, "M", nullptr, ImGuiKey_M},
	{9.25f, 0.0f, 1.0f, 1.0f, ",", ";", ImGuiKey_Comma},
	{10.25f, 0.0f, 1.0f, 1.0f, ".", ":", ImGuiKey_Period},
	{11.25f, 0.0f, 1.0f, 1.0f, "-", "_", ImGuiKey_Slash},
	{12.25f, 0.0f, 2.75f, 1.0f, "Shift", nullptr, ImGuiKey_RightShift},
};

// ISO AZERTY (French) rows
static const KeyLayoutData azerty_iso_row1_keys[] = {
	{0.0f, 0.0f, 1.5f, 1.0f, "Tab", nullptr, ImGuiKey_Tab},
	{1.5f, 0.0f, 1.0f, 1.0f, "A", nullptr, ImGuiKey_A},
	{2.5f, 0.0f, 1.0f, 1.0f, "Z", nullptr, ImGuiKey_Z},
	{3.5f, 0.0f, 1.0f, 1.0f, "E", nullptr, ImGuiKey_E},
	{4.5f, 0.0f, 1.0f, 1.0f, "R", nullptr, ImGuiKey_R},
	{5.5f, 0.0f, 1.0f, 1.0f, "T", nullptr, ImGuiKey_T},
	{6.5f, 0.0f, 1.0f, 1.0f, "Y", nullptr, ImGuiKey_Y},
	{7.5f, 0.0f, 1.0f, 1.0f, "U", nullptr, ImGuiKey_U},
	{8.5f, 0.0f, 1.0f, 1.0f, "I", nullptr, ImGuiKey_I},
	{9.5f, 0.0f, 1.0f, 1.0f, "O", nullptr, ImGuiKey_O},
	{10.5f, 0.0f, 1.0f, 1.0f, "P", nullptr, ImGuiKey_P},
	{11.5f, 0.0f, 1.0f, 1.0f, "^", nullptr, ImGuiKey_LeftBracket},
	{12.5f, 0.0f, 1.0f, 1.0f, "$", nullptr, ImGuiKey_RightBracket},
};

static const KeyLayoutData azerty_iso_row2_keys[] = {
	{0.0f, 0.0f, 1.75f, 1.0f, "Caps", nullptr, ImGuiKey_CapsLock},
	{1.75f, 0.0f, 1.0f, 1.0f, "Q", nullptr, ImGuiKey_Q},
	{2.75f, 0.0f, 1.0f, 1.0f, "S", nullptr, ImGuiKey_S},
	{3.75f, 0.0f, 1.0f, 1.0f, "D", nullptr, ImGuiKey_D},
	{4.75f, 0.0f, 1.0f, 1.0f, "F", nullptr, ImGuiKey_F},
	{5.75f, 0.0f, 1.0f, 1.0f, "G", nullptr, ImGuiKey_G},
	{6.75f, 0.0f, 1.0f, 1.0f, "H", nullptr, ImGuiKey_H},
	{7.75f, 0.0f, 1.0f, 1.0f, "J", nullptr, ImGuiKey_J},
	{8.75f, 0.0f, 1.0f, 1.0f, "K", nullptr, ImGuiKey_K},
	{9.75f, 0.0f, 1.0f, 1.0f, "L", nullptr, ImGuiKey_L},
	{10.75f, 0.0f, 1.0f, 1.0f, "M", nullptr, ImGuiKey_M},
	{11.75f, 0.0f, 1.0f, 1.0f, "\xc3\xb9", "%", ImGuiKey_Apostrophe},
	{12.75f, 0.0f, 1.0f, 1.0f, "*", nullptr, ImGuiKey_Backslash},
};

static const KeyLayoutData azerty_iso_row3_keys[] = {
	{0.0f, 0.0f, 1.25f, 1.0f, "Shift", nullptr, ImGuiKey_LeftShift},
	{1.25f, 0.0f, 1.0f, 1.0f, "<", ">", ImGuiKey_Oem102},
	{2.25f, 0.0f, 1.0f, 1.0f, "W", nullptr, ImGuiKey_W},
	{3.25f, 0.0f, 1.0f, 1.0f, "X", nullptr, ImGuiKey_X},
	{4.25f, 0.0f, 1.0f, 1.0f, "C", nullptr, ImGuiKey_C},
	{5.25f, 0.0f, 1.0f, 1.0f, "V", nullptr, ImGuiKey_V},
	{6.25f, 0.0f, 1.0f, 1.0f, "B", nullptr, ImGuiKey_B},
	{7.25f, 0.0f, 1.0f, 1.0f, "N", nullptr, ImGuiKey_N},
	{8.25f, 0.0f, 1.0f, 1.0f, ",", "?", ImGuiKey_Comma},
	{9.25f, 0.0f, 1.0f, 1.0f, ";", ".", ImGuiKey_Semicolon},
	{10.25f, 0.0f, 1.0f, 1.0f, ":", "/", ImGuiKey_Period},
	{11.25f, 0.0f, 1.0f, 1.0f, "!", nullptr, ImGuiKey_Slash},
	{12.25f, 0.0f, 2.75f, 1.0f, "Shift", nullptr, ImGuiKey_RightShift},
};

// Apple ANSI layout rows (US Mac keyboard)
// Apple keyboards use Command (⌘) instead of Win, Option instead of Alt
static const KeyLayoutData apple_ansi_row1_keys[] = {
	{0.0f, 0.0f, 1.5f, 1.0f, "Tab", nullptr, ImGuiKey_Tab},
	{1.5f, 0.0f, 1.0f, 1.0f, "Q", nullptr, ImGuiKey_Q},
	{2.5f, 0.0f, 1.0f, 1.0f, "W", nullptr, ImGuiKey_W},
	{3.5f, 0.0f, 1.0f, 1.0f, "E", nullptr, ImGuiKey_E},
	{4.5f, 0.0f, 1.0f, 1.0f, "R", nullptr, ImGuiKey_R},
	{5.5f, 0.0f, 1.0f, 1.0f, "T", nullptr, ImGuiKey_T},
	{6.5f, 0.0f, 1.0f, 1.0f, "Y", nullptr, ImGuiKey_Y},
	{7.5f, 0.0f, 1.0f, 1.0f, "U", nullptr, ImGuiKey_U},
	{8.5f, 0.0f, 1.0f, 1.0f, "I", nullptr, ImGuiKey_I},
	{9.5f, 0.0f, 1.0f, 1.0f, "O", nullptr, ImGuiKey_O},
	{10.5f, 0.0f, 1.0f, 1.0f, "P", nullptr, ImGuiKey_P},
	{11.5f, 0.0f, 1.0f, 1.0f, "[", "{", ImGuiKey_LeftBracket},
	{12.5f, 0.0f, 1.0f, 1.0f, "]", "}", ImGuiKey_RightBracket},
	{13.5f, 0.0f, 1.5f, 1.0f, "\\", "|", ImGuiKey_Backslash},
};

static const KeyLayoutData apple_ansi_row2_keys[] = {
	{0.0f, 0.0f, 1.75f, 1.0f, "Caps", nullptr, ImGuiKey_CapsLock},
	{1.75f, 0.0f, 1.0f, 1.0f, "A", nullptr, ImGuiKey_A},
	{2.75f, 0.0f, 1.0f, 1.0f, "S", nullptr, ImGuiKey_S},
	{3.75f, 0.0f, 1.0f, 1.0f, "D", nullptr, ImGuiKey_D},
	{4.75f, 0.0f, 1.0f, 1.0f, "F", nullptr, ImGuiKey_F},
	{5.75f, 0.0f, 1.0f, 1.0f, "G", nullptr, ImGuiKey_G},
	{6.75f, 0.0f, 1.0f, 1.0f, "H", nullptr, ImGuiKey_H},
	{7.75f, 0.0f, 1.0f, 1.0f, "J", nullptr, ImGuiKey_J},
	{8.75f, 0.0f, 1.0f, 1.0f, "K", nullptr, ImGuiKey_K},
	{9.75f, 0.0f, 1.0f, 1.0f, "L", nullptr, ImGuiKey_L},
	{10.75f, 0.0f, 1.0f, 1.0f, ";", ":", ImGuiKey_Semicolon},
	{11.75f, 0.0f, 1.0f, 1.0f, "'", "\"", ImGuiKey_Apostrophe},
	{12.75f, 0.0f, 2.25f, 1.0f, "Return", nullptr, ImGuiKey_Enter},
};

static const KeyLayoutData apple_ansi_row3_keys[] = {
	{0.0f, 0.0f, 2.25f, 1.0f, "Shift", nullptr, ImGuiKey_LeftShift},
	{2.25f, 0.0f, 1.0f, 1.0f, "Z", nullptr, ImGuiKey_Z},
	{3.25f, 0.0f, 1.0f, 1.0f, "X", nullptr, ImGuiKey_X},
	{4.25f, 0.0f, 1.0f, 1.0f, "C", nullptr, ImGuiKey_C},
	{5.25f, 0.0f, 1.0f, 1.0f, "V", nullptr, ImGuiKey_V},
	{6.25f, 0.0f, 1.0f, 1.0f, "B", nullptr, ImGuiKey_B},
	{7.25f, 0.0f, 1.0f, 1.0f, "N", nullptr, ImGuiKey_N},
	{8.25f, 0.0f, 1.0f, 1.0f, "M", nullptr, ImGuiKey_M},
	{9.25f, 0.0f, 1.0f, 1.0f, ",", "<", ImGuiKey_Comma},
	{10.25f, 0.0f, 1.0f, 1.0f, ".", ">", ImGuiKey_Period},
	{11.25f, 0.0f, 1.0f, 1.0f, "/", "?", ImGuiKey_Slash},
	{12.25f, 0.0f, 2.75f, 1.0f, "Shift", nullptr, ImGuiKey_RightShift},
};

// Apple full keyboard bottom row (Ctrl, Option, Command, Space, Command, Option, Ctrl)
// Full-size layout like Apple Magic Keyboard with Numeric Keypad - no Fn key, wider modifier keys
static const KeyLayoutData apple_bottom_row_keys[] = {
	{0.0f, 0.0f, 1.5f, 1.0f, "Ctrl", nullptr, ImGuiKey_LeftCtrl},
	{1.5f, 0.0f, 1.25f, 1.0f, "Opt", nullptr, ImGuiKey_LeftAlt},
	{2.75f, 0.0f, 1.5f, 1.0f, "Cmd", nullptr, ImGuiKey_LeftSuper},
	{4.25f, 0.0f, 6.25f, 1.0f, "Space", nullptr, ImGuiKey_Space},
	{10.5f, 0.0f, 1.5f, 1.0f, "Cmd", nullptr, ImGuiKey_RightSuper},
	{12.0f, 0.0f, 1.25f, 1.0f, "Opt", nullptr, ImGuiKey_RightAlt},
	{13.25f, 0.0f, 1.75f, 1.0f, "Ctrl", nullptr, ImGuiKey_RightCtrl},
};

// Apple ISO layout rows (UK/International Mac keyboard)
// Same as Apple ANSI but with ISO Enter key and extra key next to left Shift
static const KeyLayoutData apple_iso_row1_keys[] = {
	{0.0f, 0.0f, 1.5f, 1.0f, "Tab", nullptr, ImGuiKey_Tab},
	{1.5f, 0.0f, 1.0f, 1.0f, "Q", nullptr, ImGuiKey_Q},
	{2.5f, 0.0f, 1.0f, 1.0f, "W", nullptr, ImGuiKey_W},
	{3.5f, 0.0f, 1.0f, 1.0f, "E", nullptr, ImGuiKey_E},
	{4.5f, 0.0f, 1.0f, 1.0f, "R", nullptr, ImGuiKey_R},
	{5.5f, 0.0f, 1.0f, 1.0f, "T", nullptr, ImGuiKey_T},
	{6.5f, 0.0f, 1.0f, 1.0f, "Y", nullptr, ImGuiKey_Y},
	{7.5f, 0.0f, 1.0f, 1.0f, "U", nullptr, ImGuiKey_U},
	{8.5f, 0.0f, 1.0f, 1.0f, "I", nullptr, ImGuiKey_I},
	{9.5f, 0.0f, 1.0f, 1.0f, "O", nullptr, ImGuiKey_O},
	{10.5f, 0.0f, 1.0f, 1.0f, "P", nullptr, ImGuiKey_P},
	{11.5f, 0.0f, 1.0f, 1.0f, "[", "{", ImGuiKey_LeftBracket},
	{12.5f, 0.0f, 1.0f, 1.0f, "]", "}", ImGuiKey_RightBracket},
};

static const KeyLayoutData apple_iso_row2_keys[] = {
	{0.0f, 0.0f, 1.75f, 1.0f, "Caps", nullptr, ImGuiKey_CapsLock},
	{1.75f, 0.0f, 1.0f, 1.0f, "A", nullptr, ImGuiKey_A},
	{2.75f, 0.0f, 1.0f, 1.0f, "S", nullptr, ImGuiKey_S},
	{3.75f, 0.0f, 1.0f, 1.0f, "D", nullptr, ImGuiKey_D},
	{4.75f, 0.0f, 1.0f, 1.0f, "F", nullptr, ImGuiKey_F},
	{5.75f, 0.0f, 1.0f, 1.0f, "G", nullptr, ImGuiKey_G},
	{6.75f, 0.0f, 1.0f, 1.0f, "H", nullptr, ImGuiKey_H},
	{7.75f, 0.0f, 1.0f, 1.0f, "J", nullptr, ImGuiKey_J},
	{8.75f, 0.0f, 1.0f, 1.0f, "K", nullptr, ImGuiKey_K},
	{9.75f, 0.0f, 1.0f, 1.0f, "L", nullptr, ImGuiKey_L},
	{10.75f, 0.0f, 1.0f, 1.0f, ";", ":", ImGuiKey_Semicolon},
	{11.75f, 0.0f, 1.0f, 1.0f, "'", "\"", ImGuiKey_Apostrophe},
	{12.75f, 0.0f, 1.0f, 1.0f, "#", "~", ImGuiKey_Backslash},
};

static const KeyLayoutData apple_iso_row3_keys[] = {
	{0.0f, 0.0f, 1.25f, 1.0f, "Shift", nullptr, ImGuiKey_LeftShift},
	{1.25f, 0.0f, 1.0f, 1.0f, "`", "~", ImGuiKey_Oem102},
	{2.25f, 0.0f, 1.0f, 1.0f, "Z", nullptr, ImGuiKey_Z},
	{3.25f, 0.0f, 1.0f, 1.0f, "X", nullptr, ImGuiKey_X},
	{4.25f, 0.0f, 1.0f, 1.0f, "C", nullptr, ImGuiKey_C},
	{5.25f, 0.0f, 1.0f, 1.0f, "V", nullptr, ImGuiKey_V},
	{6.25f, 0.0f, 1.0f, 1.0f, "B", nullptr, ImGuiKey_B},
	{7.25f, 0.0f, 1.0f, 1.0f, "N", nullptr, ImGuiKey_N},
	{8.25f, 0.0f, 1.0f, 1.0f, "M", nullptr, ImGuiKey_M},
	{9.25f, 0.0f, 1.0f, 1.0f, ",", "<", ImGuiKey_Comma},
	{10.25f, 0.0f, 1.0f, 1.0f, ".", ">", ImGuiKey_Period},
	{11.25f, 0.0f, 1.0f, 1.0f, "/", "?", ImGuiKey_Slash},
	{12.25f, 0.0f, 2.75f, 1.0f, "Shift", nullptr, ImGuiKey_RightShift},
};

// Bottom row (modifiers + spacebar)
static const KeyLayoutData bottom_row_keys[] = {
	{0.0f, 0.0f, 1.25f, 1.0f, "Ctrl", nullptr, ImGuiKey_LeftCtrl},
	{1.25f, 0.0f, 1.25f, 1.0f, "Win", nullptr, ImGuiKey_LeftSuper},
	{2.5f, 0.0f, 1.25f, 1.0f, "Alt", nullptr, ImGuiKey_LeftAlt},
	{3.75f, 0.0f, 6.25f, 1.0f, "Space", nullptr, ImGuiKey_Space},
	{10.0f, 0.0f, 1.25f, 1.0f, "Alt", nullptr, ImGuiKey_RightAlt},
	{11.25f, 0.0f, 1.25f, 1.0f, "Win", nullptr, ImGuiKey_RightSuper},
	{12.5f, 0.0f, 1.25f, 1.0f, "Menu", nullptr, ImGuiKey_Menu},
	{13.75f, 0.0f, 1.25f, 1.0f, "Ctrl", nullptr, ImGuiKey_RightCtrl},
};

static void RenderKey(ImDrawList *draw_list, const ImVec2 &key_min, const ImVec2 &key_size, const char *label,
					  const char *shiftLabel, ImGuiKey key, float scale, ImGuiKeyboardFlags flags) {
	const ImGuiKeyboardStyle &style = GetStyle();
	const float key_rounding = style.KeyRounding * scale;
	const float key_face_rounding = style.KeyFaceRounding * scale;
	const ImVec2 key_face_pos(style.KeyFaceOffset.x * scale, style.KeyFaceOffset.y * scale);
	const ImVec2 key_label_pos(style.KeyLabelOffset.x * scale, style.KeyLabelOffset.y * scale);

	ImVec2 key_max = ImVec2(key_min.x + key_size.x, key_min.y + key_size.y);
	ImVec2 key_face_size =
		ImVec2(key_size.x - style.KeyFaceOffset.x * 2.0f * scale, key_size.y - style.KeyFaceOffset.y * 2.0f * scale);

	// Key background
	draw_list->AddRectFilled(key_min, key_max, GetColorU32(ImGuiKeyboardCol_KeyBackground), key_rounding);
	draw_list->AddRect(key_min, key_max, GetColorU32(ImGuiKeyboardCol_KeyBorder), key_rounding);

	// Key face
	ImVec2 face_min = ImVec2(key_min.x + key_face_pos.x, key_min.y + key_face_pos.y);
	ImVec2 face_max = ImVec2(face_min.x + key_face_size.x, face_min.y + key_face_size.y);
	draw_list->AddRect(face_min, face_max, GetColorU32(ImGuiKeyboardCol_KeyFaceBorder), key_face_rounding,
					   ImDrawFlags_None, style.KeyFaceBorderSize);
	draw_list->AddRectFilled(face_min, face_max, GetColorU32(ImGuiKeyboardCol_KeyFace), key_face_rounding);

	// Label rendering
	ImVec2 label_min = ImVec2(key_min.x + key_label_pos.x, key_min.y + key_label_pos.y);

	// Check if we should draw icons instead of text
	const bool showIcons = (flags & ImGuiKeyboardFlags_ShowIcons);
	const bool isWindowsKey = (key == ImGuiKey_LeftSuper || key == ImGuiKey_RightSuper);
	const bool isArrowKey = (key == ImGuiKey_UpArrow || key == ImGuiKey_DownArrow || key == ImGuiKey_LeftArrow ||
							 key == ImGuiKey_RightArrow);
	const bool isShiftKey = (key == ImGuiKey_LeftShift || key == ImGuiKey_RightShift);
	const bool isTabKey = (key == ImGuiKey_Tab);
	const bool isCapsLockKey = (key == ImGuiKey_CapsLock);
	const bool isEnterKey = (key == ImGuiKey_Enter || key == ImGuiKey_KeypadEnter);
	const bool isAppleModifier = (label && (strcmp(label, "Ctrl") == 0 || strcmp(label, "Opt") == 0 || strcmp(label, "Cmd") == 0));
	// Numpad navigation keys (when NumLock is off, these act as navigation keys)
	const bool numLockActive = ImGui::IsKeyDown(ImGuiKey_NumLock);
	const bool isNumpadArrowKey = !numLockActive && (key == ImGuiKey_Keypad8 || key == ImGuiKey_Keypad2 ||
													 key == ImGuiKey_Keypad4 || key == ImGuiKey_Keypad6);
	const bool isNumpadNavKey =
		!numLockActive && (key == ImGuiKey_Keypad7 || key == ImGuiKey_Keypad9 || key == ImGuiKey_Keypad1 ||
						   key == ImGuiKey_Keypad3 || key == ImGuiKey_Keypad0 || key == ImGuiKey_KeypadDecimal);

	if (showIcons && isWindowsKey && !isAppleModifier) {
		// Draw Windows logo (4 squares in a 2x2 grid)
		const float logo_size = ImGui::GetFontSize() * 0.9f;
		const float quad_size = logo_size * 0.45f;
		const float gap = logo_size * 0.1f;
		ImVec2 logo_min = ImVec2(label_min.x, label_min.y);
		ImU32 logo_color = GetColorU32(ImGuiKeyboardCol_KeyLabel);

		// Top-left quad
		draw_list->AddRectFilled(logo_min, ImVec2(logo_min.x + quad_size, logo_min.y + quad_size), logo_color);
		// Top-right quad
		draw_list->AddRectFilled(ImVec2(logo_min.x + quad_size + gap, logo_min.y),
								 ImVec2(logo_min.x + quad_size * 2.0f + gap, logo_min.y + quad_size), logo_color);
		// Bottom-left quad
		draw_list->AddRectFilled(ImVec2(logo_min.x, logo_min.y + quad_size + gap),
								 ImVec2(logo_min.x + quad_size, logo_min.y + quad_size * 2.0f + gap), logo_color);
		// Bottom-right quad
		draw_list->AddRectFilled(ImVec2(logo_min.x + quad_size + gap, logo_min.y + quad_size + gap),
								 ImVec2(logo_min.x + quad_size * 2.0f + gap, logo_min.y + quad_size * 2.0f + gap),
								 logo_color);
	} else if (showIcons && isArrowKey) {
		// Draw arrow triangles centered on the key face
		const float arrow_size = ImGui::GetFontSize() * 0.7f;
		ImU32 arrow_color = GetColorU32(ImGuiKeyboardCol_KeyLabel);
		ImVec2 center = ImVec2((face_min.x + face_max.x) * 0.5f, (face_min.y + face_max.y) * 0.5f);

		if (key == ImGuiKey_UpArrow) {
			// Triangle pointing up
			draw_list->AddTriangleFilled(ImVec2(center.x, center.y - arrow_size * 0.5f),
										 ImVec2(center.x - arrow_size * 0.5f, center.y + arrow_size * 0.5f),
										 ImVec2(center.x + arrow_size * 0.5f, center.y + arrow_size * 0.5f),
										 arrow_color);
		} else if (key == ImGuiKey_DownArrow) {
			// Triangle pointing down
			draw_list->AddTriangleFilled(ImVec2(center.x, center.y + arrow_size * 0.5f),
										 ImVec2(center.x - arrow_size * 0.5f, center.y - arrow_size * 0.5f),
										 ImVec2(center.x + arrow_size * 0.5f, center.y - arrow_size * 0.5f),
										 arrow_color);
		} else if (key == ImGuiKey_LeftArrow) {
			// Triangle pointing left
			draw_list->AddTriangleFilled(ImVec2(center.x - arrow_size * 0.5f, center.y),
										 ImVec2(center.x + arrow_size * 0.5f, center.y - arrow_size * 0.5f),
										 ImVec2(center.x + arrow_size * 0.5f, center.y + arrow_size * 0.5f),
										 arrow_color);
		} else if (key == ImGuiKey_RightArrow) {
			// Triangle pointing right
			draw_list->AddTriangleFilled(ImVec2(center.x + arrow_size * 0.5f, center.y),
										 ImVec2(center.x - arrow_size * 0.5f, center.y - arrow_size * 0.5f),
										 ImVec2(center.x - arrow_size * 0.5f, center.y + arrow_size * 0.5f),
										 arrow_color);
		}
	} else if (showIcons && isNumpadArrowKey) {
		// Draw number label first, then small arrow icon in corner (when NumLock is off)
		draw_list->AddText(label_min, GetColorU32(ImGuiKeyboardCol_KeyLabel), label);

		// Draw small arrow in bottom-right corner
		const float arrow_size = ImGui::GetFontSize() * 0.35f;
		ImU32 arrow_color = GetColorU32(ImGuiKeyboardCol_KeyLabel);
		ImVec2 center = ImVec2(face_max.x - arrow_size * 0.8f, face_max.y - arrow_size * 0.8f);

		if (key == ImGuiKey_Keypad8) {
			// Triangle pointing up
			draw_list->AddTriangleFilled(ImVec2(center.x, center.y - arrow_size * 0.5f),
										 ImVec2(center.x - arrow_size * 0.5f, center.y + arrow_size * 0.5f),
										 ImVec2(center.x + arrow_size * 0.5f, center.y + arrow_size * 0.5f),
										 arrow_color);
		} else if (key == ImGuiKey_Keypad2) {
			// Triangle pointing down
			draw_list->AddTriangleFilled(ImVec2(center.x, center.y + arrow_size * 0.5f),
										 ImVec2(center.x - arrow_size * 0.5f, center.y - arrow_size * 0.5f),
										 ImVec2(center.x + arrow_size * 0.5f, center.y - arrow_size * 0.5f),
										 arrow_color);
		} else if (key == ImGuiKey_Keypad4) {
			// Triangle pointing left
			draw_list->AddTriangleFilled(ImVec2(center.x - arrow_size * 0.5f, center.y),
										 ImVec2(center.x + arrow_size * 0.5f, center.y - arrow_size * 0.5f),
										 ImVec2(center.x + arrow_size * 0.5f, center.y + arrow_size * 0.5f),
										 arrow_color);
		} else if (key == ImGuiKey_Keypad6) {
			// Triangle pointing right
			draw_list->AddTriangleFilled(ImVec2(center.x + arrow_size * 0.5f, center.y),
										 ImVec2(center.x - arrow_size * 0.5f, center.y - arrow_size * 0.5f),
										 ImVec2(center.x - arrow_size * 0.5f, center.y + arrow_size * 0.5f),
										 arrow_color);
		}
	} else if (showIcons && isNumpadNavKey) {
		// Draw number label first, then small nav label in corner (when NumLock is off)
		draw_list->AddText(label_min, GetColorU32(ImGuiKeyboardCol_KeyLabel), label);

		// Draw small nav label in bottom-right corner
		const char *navLabel = nullptr;
		if (key == ImGuiKey_Keypad7)
			navLabel = "Hm";
		else if (key == ImGuiKey_Keypad9)
			navLabel = "PU";
		else if (key == ImGuiKey_Keypad1)
			navLabel = "En";
		else if (key == ImGuiKey_Keypad3)
			navLabel = "PD";
		else if (key == ImGuiKey_Keypad0)
			navLabel = "In";
		else if (key == ImGuiKey_KeypadDecimal)
			navLabel = "De";
		if (navLabel) {
			ImVec2 text_size = ImGui::CalcTextSize(navLabel);
			ImVec2 nav_pos = ImVec2(face_max.x - text_size.x - 2.0f, face_max.y - text_size.y - 2.0f);
			draw_list->AddText(nav_pos, GetColorU32(ImGuiKeyboardCol_KeyLabel), navLabel);
		}
	} else if (showIcons && isAppleModifier) {
		// Draw Apple modifier icons: Control, Option, Command
		const float icon_size = ImGui::GetFontSize() * 0.8f;
		ImU32 icon_color = GetColorU32(ImGuiKeyboardCol_KeyLabel);
		ImVec2 center = ImVec2(label_min.x + icon_size * 0.5f, label_min.y + icon_size * 0.5f);
		const float half = icon_size * 0.5f;
		const float thickness = icon_size * 0.12f;

		if (strcmp(label, "Ctrl") == 0) {
			// Control symbol - caret/chevron pointing up
			draw_list->AddTriangle(
				ImVec2(center.x, center.y - half * 0.6f),
				ImVec2(center.x - half * 0.7f, center.y + half * 0.3f),
				ImVec2(center.x + half * 0.7f, center.y + half * 0.3f),
				icon_color, thickness * 1.5f);
		} else if (strcmp(label, "Opt") == 0) {
			// Option symbol - split horizontal line with diagonal
			// Top right horizontal segment
			draw_list->AddLine(
				ImVec2(center.x + half * 0.1f, center.y - half * 0.4f),
				ImVec2(center.x + half * 0.8f, center.y - half * 0.4f),
				icon_color, thickness * 1.5f);
			// Diagonal from top right going down-left
			draw_list->AddLine(
				ImVec2(center.x + half * 0.1f, center.y - half * 0.4f),
				ImVec2(center.x - half * 0.4f, center.y + half * 0.3f),
				icon_color, thickness * 1.5f);
			// Bottom left horizontal segment from diagonal end
			draw_list->AddLine(
				ImVec2(center.x - half * 0.8f, center.y + half * 0.3f),
				ImVec2(center.x - half * 0.4f, center.y + half * 0.3f),
				icon_color, thickness * 1.5f);
			// Top left short horizontal line
			draw_list->AddLine(
				ImVec2(center.x - half * 0.8f, center.y - half * 0.4f),
				ImVec2(center.x - half * 0.1f, center.y - half * 0.4f),
				icon_color, thickness * 1.5f);
		} else if (strcmp(label, "Cmd") == 0) {
			// Command symbol - four loops
			const float loopSize = half * 0.35f;
			const float loopOffset = half * 0.4f;
			// Four corner loops
			draw_list->AddCircle(ImVec2(center.x - loopOffset, center.y - loopOffset), loopSize, icon_color, 12, thickness * 1.5f);
			draw_list->AddCircle(ImVec2(center.x + loopOffset, center.y - loopOffset), loopSize, icon_color, 12, thickness * 1.5f);
			draw_list->AddCircle(ImVec2(center.x - loopOffset, center.y + loopOffset), loopSize, icon_color, 12, thickness * 1.5f);
			draw_list->AddCircle(ImVec2(center.x + loopOffset, center.y + loopOffset), loopSize, icon_color, 12, thickness * 1.5f);
			// Cross connecting the loops
			draw_list->AddLine(ImVec2(center.x - loopOffset, center.y - loopOffset - loopSize),
							   ImVec2(center.x - loopOffset, center.y + loopOffset + loopSize), icon_color, thickness * 1.5f);
			draw_list->AddLine(ImVec2(center.x + loopOffset, center.y - loopOffset - loopSize),
							   ImVec2(center.x + loopOffset, center.y + loopOffset + loopSize), icon_color, thickness * 1.5f);
			draw_list->AddLine(ImVec2(center.x - loopOffset - loopSize, center.y - loopOffset),
							   ImVec2(center.x + loopOffset + loopSize, center.y - loopOffset), icon_color, thickness * 1.5f);
			draw_list->AddLine(ImVec2(center.x - loopOffset - loopSize, center.y + loopOffset),
							   ImVec2(center.x + loopOffset + loopSize, center.y + loopOffset), icon_color, thickness * 1.5f);
		}
	} else if (showIcons && isShiftKey) {
		// Draw Shift icon
		const float icon_size = ImGui::GetFontSize() * 0.8f;
		ImU32 icon_color = GetColorU32(ImGuiKeyboardCol_KeyLabel);
		ImVec2 center = ImVec2(label_min.x + icon_size * 0.5f, label_min.y + icon_size * 0.5f);
		const float half = icon_size * 0.5f;
		const float thickness = icon_size * 0.15f;

		// Outer triangle (arrow head)
		draw_list->AddTriangleFilled(ImVec2(center.x, center.y - half), ImVec2(center.x - half, center.y + half * 0.2f),
									 ImVec2(center.x + half, center.y + half * 0.2f), icon_color);
		// Stem rectangle
		draw_list->AddRectFilled(ImVec2(center.x - thickness, center.y + half * 0.2f),
								 ImVec2(center.x + thickness, center.y + half), icon_color);
	} else if (showIcons && isTabKey) {
		// Draw Tab icon (arrow pointing right with vertical bar)
		const float icon_size = ImGui::GetFontSize() * 0.8f;
		ImU32 icon_color = GetColorU32(ImGuiKeyboardCol_KeyLabel);
		ImVec2 center = ImVec2(label_min.x + icon_size * 0.5f, label_min.y + icon_size * 0.5f);
		const float half = icon_size * 0.5f;
		const float thickness = icon_size * 0.12f;

		// Horizontal line
		draw_list->AddRectFilled(ImVec2(center.x - half, center.y - thickness * 0.5f),
								 ImVec2(center.x + half * 0.5f, center.y + thickness * 0.5f), icon_color);
		// Arrow head (triangle pointing right)
		draw_list->AddTriangleFilled(ImVec2(center.x + half * 0.5f, center.y - half * 0.4f),
									 ImVec2(center.x + half * 0.5f, center.y + half * 0.4f),
									 ImVec2(center.x + half, center.y), icon_color);
		// Vertical bar at end
		draw_list->AddRectFilled(ImVec2(center.x + half - thickness, center.y - half * 0.5f),
								 ImVec2(center.x + half, center.y + half * 0.5f), icon_color);
	} else if (showIcons && isCapsLockKey) {
		// Draw Caps Lock icon (upward arrow with horizontal bar underneath)
		const float icon_size = ImGui::GetFontSize() * 0.8f;
		ImU32 icon_color = GetColorU32(ImGuiKeyboardCol_KeyLabel);
		ImVec2 center = ImVec2(label_min.x + icon_size * 0.5f, label_min.y + icon_size * 0.5f);
		const float half = icon_size * 0.5f;
		const float thickness = icon_size * 0.15f;

		// Arrow head pointing up
		draw_list->AddTriangleFilled(ImVec2(center.x, center.y - half),
									 ImVec2(center.x - half * 0.6f, center.y - half * 0.1f),
									 ImVec2(center.x + half * 0.6f, center.y - half * 0.1f), icon_color);
		// Stem
		draw_list->AddRectFilled(ImVec2(center.x - thickness, center.y - half * 0.1f),
								 ImVec2(center.x + thickness, center.y + half * 0.4f), icon_color);
		// Horizontal bar underneath
		draw_list->AddRectFilled(ImVec2(center.x - half * 0.5f, center.y + half * 0.6f),
								 ImVec2(center.x + half * 0.5f, center.y + half * 0.8f), icon_color);
	} else if (showIcons && isEnterKey) {
		// Draw Enter icon
		const float icon_size = ImGui::GetFontSize() * 0.8f;
		ImU32 icon_color = GetColorU32(ImGuiKeyboardCol_KeyLabel);
		ImVec2 start = ImVec2(label_min.x, label_min.y);
		const float thickness = icon_size * 0.12f;

		// Vertical line going up on the right
		draw_list->AddRectFilled(ImVec2(start.x + icon_size * 0.8f - thickness, start.y),
								 ImVec2(start.x + icon_size * 0.8f, start.y + icon_size * 0.5f), icon_color);
		// Horizontal line going left
		draw_list->AddRectFilled(ImVec2(start.x + icon_size * 0.15f, start.y + icon_size * 0.5f - thickness),
								 ImVec2(start.x + icon_size * 0.8f, start.y + icon_size * 0.5f), icon_color);
		// Arrow head pointing left
		draw_list->AddTriangleFilled(
			ImVec2(start.x, start.y + icon_size * 0.5f - thickness * 0.5f),
			ImVec2(start.x + icon_size * 0.25f, start.y + icon_size * 0.5f - icon_size * 0.25f),
			ImVec2(start.x + icon_size * 0.25f, start.y + icon_size * 0.5f + icon_size * 0.15f), icon_color);
	} else if ((flags & ImGuiKeyboardFlags_ShowBothLabels) && shiftLabel) {
		// Show both labels: shift label on top, normal label below
		const float lineHeight = ImGui::GetFontSize();
		draw_list->AddText(label_min, GetColorU32(ImGuiKeyboardCol_KeyLabel), shiftLabel);
		ImVec2 lower_label_min = ImVec2(label_min.x, label_min.y + lineHeight);
		draw_list->AddText(lower_label_min, GetColorU32(ImGuiKeyboardCol_KeyLabel), label);
	} else {
		// Select label based on shift state (unless NoShiftLabels flag is set)
		const bool shiftPressed = !(flags & ImGuiKeyboardFlags_NoShiftLabels) &&
								  (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift));
		const char *displayLabel = (shiftPressed && shiftLabel) ? shiftLabel : label;
		draw_list->AddText(label_min, GetColorU32(ImGuiKeyboardCol_KeyLabel), displayLabel);
	}

	// Highlight if pressed (red) or explicitly highlighted (green) or recorded (blue)
	const bool isPressed = (flags & ImGuiKeyboardFlags_ShowPressed) && key != ImGuiKey_None && ImGui::IsKeyDown(key);
	const bool isHighlighted = key != ImGuiKey_None && IsKeyHighlighted(key);
	const bool isRecorded = (flags & ImGuiKeyboardFlags_Recordable) && key != ImGuiKey_None && IsKeyRecorded(key);
	if (isPressed && isHighlighted) {
		draw_list->AddRectFilled(key_min, key_max, GetColorU32(ImGuiKeyboardCol_KeyPressedHighlighted), key_rounding);
	} else if (isPressed) {
		draw_list->AddRectFilled(key_min, key_max, GetColorU32(ImGuiKeyboardCol_KeyPressed), key_rounding);
	} else if (isHighlighted) {
		draw_list->AddRectFilled(key_min, key_max, GetColorU32(ImGuiKeyboardCol_KeyHighlighted), key_rounding);
	} else if (isRecorded) {
		draw_list->AddRectFilled(key_min, key_max, GetColorU32(ImGuiKeyboardCol_KeyRecorded), key_rounding);
	}
}

// Render ISO L-shaped Enter key as a polygon
// The ISO Enter key spans row1 and row2 with an L shape:
//   +--------+
//   |  Enter |  <- row1: wider top part
//   +--+     |
//      |     |  <- row2: narrower bottom part (right-aligned with top)
//      +-----+
static void RenderISOEnterKey(ImDrawList *draw_list, const ImVec2 &row1_pos, const ImVec2 &row2_pos,
							  float key_unit, float scale, ImGuiKeyboardFlags flags) {
	const ImGuiKeyboardStyle &style = GetStyle();
	const float key_rounding = style.KeyRounding * scale;
	const float border_size = style.KeyBorderSize * scale;

	// ISO Enter key position: right edge aligns at 15 units (standard keyboard width)
	// Top part (row1): starts at 13.5 (after ] key), width 1.5, ends at 15.0
	// Bottom part (row2): starts at 13.75 (after # key), width 1.25, ends at 15.0
	const float right_edge = 15.0f;
	const float top_start = 13.5f;
	const float bottom_start = 13.75f;

	// Key outer bounds (with border offset)
	float key_top = row1_pos.y + border_size;
	float key_row1_bottom = row1_pos.y + key_unit - border_size;
	float key_bottom = row2_pos.y + key_unit - border_size;
	float key_left_top = row1_pos.x + top_start * key_unit + border_size;       // Left edge of top part
	float key_left_bottom = row2_pos.x + bottom_start * key_unit + border_size; // Left edge of bottom part (step)
	float key_right = row1_pos.x + right_edge * key_unit - border_size;         // Right edge (aligned)

	// Draw the L-shape as two overlapping filled rectangles
	ImU32 bg_color = GetColorU32(ImGuiKeyboardCol_KeyBackground);
	ImU32 border_color = GetColorU32(ImGuiKeyboardCol_KeyBorder);

	// Draw main vertical rectangle (right side, full height)
	draw_list->AddRectFilled(ImVec2(key_left_bottom, key_top), ImVec2(key_right, key_bottom), bg_color, key_rounding);
	// Draw top-left extension (the wider top part)
	draw_list->AddRectFilled(ImVec2(key_left_top, key_top), ImVec2(key_left_bottom, key_row1_bottom), bg_color, key_rounding);

	// Draw the L-shape border using line segments (clockwise from top-left)
	// 1. Top edge (full width of top part)
	draw_list->AddLine(ImVec2(key_left_top, key_top), ImVec2(key_right, key_top), border_color);
	// 2. Right edge (full height)
	draw_list->AddLine(ImVec2(key_right, key_top), ImVec2(key_right, key_bottom), border_color);
	// 3. Bottom edge (width of bottom part)
	draw_list->AddLine(ImVec2(key_right, key_bottom), ImVec2(key_left_bottom, key_bottom), border_color);
	// 4. Left edge of bottom part (going up to step)
	draw_list->AddLine(ImVec2(key_left_bottom, key_bottom), ImVec2(key_left_bottom, key_row1_bottom), border_color);
	// 5. Step edge (horizontal, going left)
	draw_list->AddLine(ImVec2(key_left_bottom, key_row1_bottom), ImVec2(key_left_top, key_row1_bottom), border_color);
	// 6. Left edge of top part (going up to close)
	draw_list->AddLine(ImVec2(key_left_top, key_row1_bottom), ImVec2(key_left_top, key_top), border_color);

	// Key face (inner raised area)
	const ImVec2 face_offset(style.KeyFaceOffset.x * scale, style.KeyFaceOffset.y * scale);
	ImU32 face_color = GetColorU32(ImGuiKeyboardCol_KeyFace);

	float face_top = key_top + face_offset.y;
	float face_row1_bottom = key_row1_bottom - face_offset.y;
	float face_bottom = key_bottom - face_offset.y;
	float face_left_top = key_left_top + face_offset.x;
	float face_left_bottom = key_left_bottom + face_offset.x;
	float face_right = key_right - face_offset.x;

	// Draw face as two rectangles
	draw_list->AddRectFilled(ImVec2(face_left_bottom, face_top), ImVec2(face_right, face_bottom), face_color);
	draw_list->AddRectFilled(ImVec2(face_left_top, face_top), ImVec2(face_left_bottom, face_row1_bottom), face_color);

	// Label "Enter" - position in top part of the L-shape
	float top_center_x = (key_left_top + key_right) * 0.5f;
	float top_center_y = (key_top + key_row1_bottom) * 0.5f;
	const char *label = "Enter";
	ImVec2 text_size = ImGui::CalcTextSize(label);
	ImVec2 label_pos(top_center_x - text_size.x * 0.5f, top_center_y - text_size.y * 0.5f);
	draw_list->AddText(label_pos, GetColorU32(ImGuiKeyboardCol_KeyLabel), label);

	// Draw Enter arrow icon (↵) in the bottom part of the L-shape
	ImU32 icon_color = GetColorU32(ImGuiKeyboardCol_KeyLabel);
	float bottom_center_x = (key_left_bottom + key_right) * 0.5f;
	float bottom_center_y = (key_row1_bottom + key_bottom) * 0.5f;
	float icon_size = ImGui::GetFontSize() * 0.6f;

	// Draw the bent arrow: vertical line going up, then horizontal line going left with arrowhead
	// Arrow shape: |
	//              +--<
	float arrow_top = bottom_center_y - icon_size * 0.4f;
	float arrow_bottom = bottom_center_y + icon_size * 0.3f;
	float arrow_right = bottom_center_x + icon_size * 0.4f;
	float arrow_left = bottom_center_x - icon_size * 0.4f;
	float arrow_bend_y = arrow_bottom;

	// Vertical line (going down from top)
	draw_list->AddLine(ImVec2(arrow_right, arrow_top), ImVec2(arrow_right, arrow_bend_y), icon_color, 1.5f * scale);
	// Horizontal line (going left)
	draw_list->AddLine(ImVec2(arrow_right, arrow_bend_y), ImVec2(arrow_left, arrow_bend_y), icon_color, 1.5f * scale);
	// Arrowhead pointing left
	float arrow_head_size = icon_size * 0.25f;
	draw_list->AddTriangleFilled(
		ImVec2(arrow_left, arrow_bend_y),
		ImVec2(arrow_left + arrow_head_size, arrow_bend_y - arrow_head_size),
		ImVec2(arrow_left + arrow_head_size, arrow_bend_y + arrow_head_size),
		icon_color);

	// Highlight overlay if pressed/highlighted/recorded
	const bool isPressed = (flags & ImGuiKeyboardFlags_ShowPressed) && ImGui::IsKeyDown(ImGuiKey_Enter);
	const bool isHighlighted = IsKeyHighlighted(ImGuiKey_Enter);
	const bool isRecorded = (flags & ImGuiKeyboardFlags_Recordable) && IsKeyRecorded(ImGuiKey_Enter);

	ImU32 highlight_color = 0;
	if (isPressed && isHighlighted) {
		highlight_color = GetColorU32(ImGuiKeyboardCol_KeyPressedHighlighted);
	} else if (isPressed) {
		highlight_color = GetColorU32(ImGuiKeyboardCol_KeyPressed);
	} else if (isHighlighted) {
		highlight_color = GetColorU32(ImGuiKeyboardCol_KeyHighlighted);
	} else if (isRecorded) {
		highlight_color = GetColorU32(ImGuiKeyboardCol_KeyRecorded);
	}

	if (highlight_color != 0) {
		draw_list->AddRectFilled(ImVec2(key_left_bottom, key_top), ImVec2(key_right, key_bottom), highlight_color, key_rounding);
		draw_list->AddRectFilled(ImVec2(key_left_top, key_top), ImVec2(key_left_bottom, key_row1_bottom), highlight_color, key_rounding);
	}
}

// Check if mouse is inside the ISO Enter key L-shape
static bool IsMouseInISOEnterKey(const ImVec2 &mouse_pos, const ImVec2 &row1_pos, const ImVec2 &row2_pos, float key_unit) {
	// ISO Enter key position: right edge aligns at 15 units
	// Top part: starts at 13.5 (after ] key), width 1.5
	// Bottom part: starts at 13.75 (after # key), width 1.25
	const float right_edge = 15.0f;
	const float top_start = 13.5f;
	const float bottom_start = 13.75f;

	// Top rectangle bounds
	float top_left = row1_pos.x + top_start * key_unit;
	float top_right = row1_pos.x + right_edge * key_unit;
	float top_top = row1_pos.y;
	float row1_bottom = row1_pos.y + key_unit;

	// Bottom rectangle bounds
	float bottom_left = row2_pos.x + bottom_start * key_unit;
	float bottom_right = row2_pos.x + right_edge * key_unit;
	float bottom_top = row2_pos.y;
	float bottom_bottom = row2_pos.y + key_unit;

	// Check if in top part (row1)
	if (mouse_pos.x >= top_left && mouse_pos.x <= top_right &&
		mouse_pos.y >= top_top && mouse_pos.y <= row1_bottom) {
		return true;
	}

	// Check if in bottom part (row2)
	if (mouse_pos.x >= bottom_left && mouse_pos.x <= bottom_right &&
		mouse_pos.y >= bottom_top && mouse_pos.y <= bottom_bottom) {
		return true;
	}

	return false;
}

static void RenderKeyRow(ImDrawList *draw_list, const KeyLayoutData *keys, int key_count, const ImVec2 &start_pos,
						 float key_unit, float scale, ImGuiKeyboardFlags flags) {
	const ImGuiKeyboardStyle &style = GetStyle();
	const float border_size = style.KeyBorderSize * scale;
	for (int i = 0; i < key_count; i++) {
		const KeyLayoutData *key = &keys[i];
		ImVec2 key_min =
			ImVec2(start_pos.x + key->X * key_unit + border_size, start_pos.y + key->Y * key_unit + border_size);
		ImVec2 key_size =
			ImVec2(key->Width * key_unit - 2.0f * border_size, key->Height * key_unit - 2.0f * border_size);
		RenderKey(draw_list, key_min, key_size, key->Label, key->ShiftLabel, key->Key, scale, flags);
	}
}

static void Record(ImGuiKey key, bool record) {
	KeyboardContext *ctx = GetContext();
	if (record) {
		// Add key if not already present
		if (!IsKeyRecorded(key)) {
			ctx->RecordedKeys.push_back(key);
		}
	} else {
		// Remove key if present
		for (int i = 0; i < ctx->RecordedKeys.Size; i++) {
			if (ctx->RecordedKeys[i] == key) {
				ctx->RecordedKeys.erase(&ctx->RecordedKeys[i]);
				break;
			}
		}
	}
}

// Handle recording for a row of keys - called when Recordable flag is set
static void HandleKeyRowRecording(const KeyLayoutData *keys, int key_count, const ImVec2 &start_pos,
								   float key_unit, float scale, const ImVec2 &mouse_pos, bool mouse_clicked) {
	const ImGuiKeyboardStyle &style = GetStyle();
	const float border_size = style.KeyBorderSize * scale;
	for (int i = 0; i < key_count; i++) {
		const KeyLayoutData *key = &keys[i];
		ImVec2 key_min =
			ImVec2(start_pos.x + key->X * key_unit + border_size, start_pos.y + key->Y * key_unit + border_size);
		ImVec2 key_size =
			ImVec2(key->Width * key_unit - 2.0f * border_size, key->Height * key_unit - 2.0f * border_size);
		ImVec2 key_max = ImVec2(key_min.x + key_size.x, key_min.y + key_size.y);

		// Check if mouse is inside this key and was clicked
		if (mouse_clicked &&
			mouse_pos.x >= key_min.x && mouse_pos.x < key_max.x &&
			mouse_pos.y >= key_min.y && mouse_pos.y < key_max.y) {
			// Toggle the recorded state
			if (IsKeyRecorded(key->Key)) {
				Record(key->Key, false);
			} else {
				Record(key->Key, true);
			}
		}
	}
}

void Highlight(ImGuiKey key, bool highlight) {
	KeyboardContext *ctx = GetContext();
	if (highlight) {
		// Add key if not already present
		if (!IsKeyHighlighted(key)) {
			ctx->HighlightedKeys.push_back(key);
		}
	} else {
		// Remove key if present
		for (int i = 0; i < ctx->HighlightedKeys.Size; i++) {
			if (ctx->HighlightedKeys[i] == key) {
				ctx->HighlightedKeys.erase(&ctx->HighlightedKeys[i]);
				break;
			}
		}
	}
}

void ClearHighlights() {
	KeyboardContext *ctx = GetContext();
	ctx->HighlightedKeys.clear();
}

void ClearRecorded() {
	KeyboardContext *ctx = GetContext();
	ctx->RecordedKeys.clear();
}

const ImVector<ImGuiKey>& GetRecordedKeys() {
	return GetContext()->RecordedKeys;
}

void Keyboard(ImGuiKeyboardLayout layout, ImGuiKeyboardFlags flags) {
	IM_ASSERT(!(layout == ImGuiKeyboardLayout_NumericPad && (flags & ImGuiKeyboardFlags_NoNumpad)) &&
			  "Cannot use NoNumpad flag with NumericPad layout");

	KeyboardContext *ctx = GetContext();
	const ImGuiKeyboardStyle &style = ctx->Style;
	ImDrawList *draw_list = ImGui::GetWindowDrawList();
	const float scale = ImGui::GetFontSize() / 13.0f;
	const float key_unit = style.KeyUnit * scale;
	const float section_gap = style.SectionGap * scale;
	const float board_padding = style.BoardPadding * scale;

	ImVec2 board_min = ImGui::GetCursorScreenPos();

	// Calculate board dimensions based on layout
	float board_width, board_height;
	if (layout == ImGuiKeyboardLayout_NumericPad) {
		// Numpad only: 4 keys wide, 5 rows
		board_width = 4.0f * key_unit + board_padding * 2.0f;
		board_height = 5.0f * key_unit + board_padding * 2.0f;
	} else {
		// Full keyboard: main section (15 keys) + nav cluster (3 keys) + numpad (4 keys) + gaps
		float numpad_width = (flags & ImGuiKeyboardFlags_NoNumpad) ? 0.0f : (section_gap + 4.0f * key_unit);
		board_width = 15.0f * key_unit + section_gap + 3.0f * key_unit + numpad_width + board_padding * 2.0f;
		board_height = 6.5f * key_unit + board_padding * 2.0f; // Function row + gap + 5 main rows
	}

	ImVec2 board_max = ImVec2(board_min.x + board_width, board_min.y + board_height);
	ImVec2 start_pos = ImVec2(board_min.x + board_padding, board_min.y + board_padding);

	// Reserve space and check visibility
	ImGui::Dummy(ImVec2(board_width, board_height));
	if (!ImGui::IsItemVisible()) {
		return;
	}

	// Handle recording when Recordable flag is set
	const bool recordable = (flags & ImGuiKeyboardFlags_Recordable) != 0;
	bool mouse_clicked = false;
	ImVec2 mouse_pos;
	if (recordable) {
		mouse_pos = ImGui::GetMousePos();
		// Check if the mouse is within the board bounds and left button was clicked
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
			mouse_pos.x >= board_min.x && mouse_pos.x < board_max.x &&
			mouse_pos.y >= board_min.y && mouse_pos.y < board_max.y) {
			mouse_clicked = true;
		}

		// Also detect actual keyboard key presses and toggle their recorded state
		// Skip mouse buttons - they shouldn't be recorded when clicking on virtual keys
		for (int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; k++) {
			ImGuiKey key = (ImGuiKey)k;
			// Skip mouse buttons and mouse wheel
			if (!ImGui::IsKeyboardKey(key)) {
				continue;
			}
			if (ImGui::IsKeyPressed(key, false)) {
				// Toggle the recorded state
				if (IsKeyRecorded(key)) {
					Record(key, false);
				} else {
					Record(key, true);
				}
			}
		}
	}

	// Draw board background
	const float board_rounding = style.BoardRounding * scale;
	draw_list->AddRectFilled(board_min, board_max, GetColorU32(ImGuiKeyboardCol_BoardBackground), board_rounding);

	draw_list->PushClipRect(board_min, board_max, true);

	if (layout == ImGuiKeyboardLayout_NumericPad) {
		// Render only numpad
		RenderKeyRow(draw_list, numpad_keys, IM_ARRAYSIZE(numpad_keys), start_pos, key_unit, scale, flags);
		// Handle recording for numpad
		if (mouse_clicked) {
			HandleKeyRowRecording(numpad_keys, IM_ARRAYSIZE(numpad_keys), start_pos, key_unit, scale, mouse_pos, mouse_clicked);
		}
	} else {
		// Full keyboard rendering

		// Navigation cluster X position (used for Print/Scroll/Pause alignment)
		float nav_x = start_pos.x + 15.0f * key_unit + section_gap;

		// Function row (with gap below)
		ImVec2 func_row_pos = start_pos;
		RenderKeyRow(draw_list, function_row_keys, IM_ARRAYSIZE(function_row_keys), func_row_pos, key_unit, scale,
					 flags);
		// Print/Scroll/Pause (or F13/F14/F15 for Apple) - aligned with nav cluster
		ImVec2 func_row_nav_pos = ImVec2(nav_x, start_pos.y);
		const bool useAppleFunctionNav = (layout == ImGuiKeyboardLayout_AppleANSI || layout == ImGuiKeyboardLayout_AppleISO);
		if (useAppleFunctionNav) {
			RenderKeyRow(draw_list, apple_function_row_nav_keys, IM_ARRAYSIZE(apple_function_row_nav_keys), func_row_nav_pos, key_unit, scale, flags);
		} else {
			RenderKeyRow(draw_list, function_row_nav_keys, IM_ARRAYSIZE(function_row_nav_keys), func_row_nav_pos, key_unit, scale, flags);
		}

		// Main keyboard section (offset by function row + gap)
		float main_section_y = start_pos.y + key_unit + 0.5f * key_unit;

		// Number row - select based on layout
		ImVec2 num_row_pos = ImVec2(start_pos.x, main_section_y);
		const KeyLayoutData *num_row_keys;
		int num_row_count;
		switch (layout) {
		case ImGuiKeyboardLayout_Qwertz:
		case ImGuiKeyboardLayout_QwertzISO:
			num_row_keys = number_row_qwertz_keys;
			num_row_count = IM_ARRAYSIZE(number_row_qwertz_keys);
			break;
		case ImGuiKeyboardLayout_Azerty:
		case ImGuiKeyboardLayout_AzertyISO:
			num_row_keys = number_row_azerty_keys;
			num_row_count = IM_ARRAYSIZE(number_row_azerty_keys);
			break;
		default:
			num_row_keys = number_row_keys;
			num_row_count = IM_ARRAYSIZE(number_row_keys);
			break;
		}
		RenderKeyRow(draw_list, num_row_keys, num_row_count, num_row_pos, key_unit, scale, flags);

		// Letter rows - select based on layout
		const KeyLayoutData *row1_keys;
		const KeyLayoutData *row2_keys;
		const KeyLayoutData *row3_keys;
		int row1_count, row2_count, row3_count;

		switch (layout) {
		case ImGuiKeyboardLayout_Qwertz:
			row1_keys = qwertz_row1_keys;
			row1_count = IM_ARRAYSIZE(qwertz_row1_keys);
			row2_keys = qwertz_row2_keys;
			row2_count = IM_ARRAYSIZE(qwertz_row2_keys);
			row3_keys = qwertz_row3_keys;
			row3_count = IM_ARRAYSIZE(qwertz_row3_keys);
			break;
		case ImGuiKeyboardLayout_Azerty:
			row1_keys = azerty_row1_keys;
			row1_count = IM_ARRAYSIZE(azerty_row1_keys);
			row2_keys = azerty_row2_keys;
			row2_count = IM_ARRAYSIZE(azerty_row2_keys);
			row3_keys = azerty_row3_keys;
			row3_count = IM_ARRAYSIZE(azerty_row3_keys);
			break;
		case ImGuiKeyboardLayout_Colemak:
			row1_keys = colemak_row1_keys;
			row1_count = IM_ARRAYSIZE(colemak_row1_keys);
			row2_keys = colemak_row2_keys;
			row2_count = IM_ARRAYSIZE(colemak_row2_keys);
			row3_keys = colemak_row3_keys;
			row3_count = IM_ARRAYSIZE(colemak_row3_keys);
			break;
		case ImGuiKeyboardLayout_Dvorak:
			row1_keys = dvorak_row1_keys;
			row1_count = IM_ARRAYSIZE(dvorak_row1_keys);
			row2_keys = dvorak_row2_keys;
			row2_count = IM_ARRAYSIZE(dvorak_row2_keys);
			row3_keys = dvorak_row3_keys;
			row3_count = IM_ARRAYSIZE(dvorak_row3_keys);
			break;
		case ImGuiKeyboardLayout_QwertyISO:
			row1_keys = qwerty_iso_row1_keys;
			row1_count = IM_ARRAYSIZE(qwerty_iso_row1_keys);
			row2_keys = qwerty_iso_row2_keys;
			row2_count = IM_ARRAYSIZE(qwerty_iso_row2_keys);
			row3_keys = qwerty_iso_row3_keys;
			row3_count = IM_ARRAYSIZE(qwerty_iso_row3_keys);
			break;
		case ImGuiKeyboardLayout_QwertzISO:
			row1_keys = qwertz_iso_row1_keys;
			row1_count = IM_ARRAYSIZE(qwertz_iso_row1_keys);
			row2_keys = qwertz_iso_row2_keys;
			row2_count = IM_ARRAYSIZE(qwertz_iso_row2_keys);
			row3_keys = qwertz_iso_row3_keys;
			row3_count = IM_ARRAYSIZE(qwertz_iso_row3_keys);
			break;
		case ImGuiKeyboardLayout_AzertyISO:
			row1_keys = azerty_iso_row1_keys;
			row1_count = IM_ARRAYSIZE(azerty_iso_row1_keys);
			row2_keys = azerty_iso_row2_keys;
			row2_count = IM_ARRAYSIZE(azerty_iso_row2_keys);
			row3_keys = azerty_iso_row3_keys;
			row3_count = IM_ARRAYSIZE(azerty_iso_row3_keys);
			break;
		case ImGuiKeyboardLayout_AppleANSI:
			row1_keys = apple_ansi_row1_keys;
			row1_count = IM_ARRAYSIZE(apple_ansi_row1_keys);
			row2_keys = apple_ansi_row2_keys;
			row2_count = IM_ARRAYSIZE(apple_ansi_row2_keys);
			row3_keys = apple_ansi_row3_keys;
			row3_count = IM_ARRAYSIZE(apple_ansi_row3_keys);
			break;
		case ImGuiKeyboardLayout_AppleISO:
			row1_keys = apple_iso_row1_keys;
			row1_count = IM_ARRAYSIZE(apple_iso_row1_keys);
			row2_keys = apple_iso_row2_keys;
			row2_count = IM_ARRAYSIZE(apple_iso_row2_keys);
			row3_keys = apple_iso_row3_keys;
			row3_count = IM_ARRAYSIZE(apple_iso_row3_keys);
			break;
		default: // QWERTY
			row1_keys = qwerty_row1_keys;
			row1_count = IM_ARRAYSIZE(qwerty_row1_keys);
			row2_keys = qwerty_row2_keys;
			row2_count = IM_ARRAYSIZE(qwerty_row2_keys);
			row3_keys = qwerty_row3_keys;
			row3_count = IM_ARRAYSIZE(qwerty_row3_keys);
			break;
		}

		ImVec2 row1_pos = ImVec2(start_pos.x, main_section_y + key_unit);
		RenderKeyRow(draw_list, row1_keys, row1_count, row1_pos, key_unit, scale, flags);

		ImVec2 row2_pos = ImVec2(start_pos.x, main_section_y + 2.0f * key_unit);
		RenderKeyRow(draw_list, row2_keys, row2_count, row2_pos, key_unit, scale, flags);

		// Render ISO L-shaped Enter key for ISO layouts
		const bool isISOLayout = (layout == ImGuiKeyboardLayout_QwertyISO ||
								  layout == ImGuiKeyboardLayout_QwertzISO ||
								  layout == ImGuiKeyboardLayout_AzertyISO ||
								  layout == ImGuiKeyboardLayout_AppleISO);
		if (isISOLayout) {
			RenderISOEnterKey(draw_list, row1_pos, row2_pos, key_unit, scale, flags);
		}

		ImVec2 row3_pos = ImVec2(start_pos.x, main_section_y + 3.0f * key_unit);
		RenderKeyRow(draw_list, row3_keys, row3_count, row3_pos, key_unit, scale, flags);

		// Bottom row (modifiers + spacebar) - use Apple bottom row for Apple layouts
		ImVec2 bottom_row_pos = ImVec2(start_pos.x, main_section_y + 4.0f * key_unit);
		const bool isAppleLayout = (layout == ImGuiKeyboardLayout_AppleANSI ||
									layout == ImGuiKeyboardLayout_AppleISO);
		if (isAppleLayout) {
			RenderKeyRow(draw_list, apple_bottom_row_keys, IM_ARRAYSIZE(apple_bottom_row_keys), bottom_row_pos, key_unit, scale, flags);
		} else {
			RenderKeyRow(draw_list, bottom_row_keys, IM_ARRAYSIZE(bottom_row_keys), bottom_row_pos, key_unit, scale, flags);
		}

		// Navigation cluster (Insert/Delete/Home/End/PgUp/PgDn + arrows)
		ImVec2 nav_pos = ImVec2(nav_x, main_section_y);
		RenderKeyRow(draw_list, nav_cluster_keys, IM_ARRAYSIZE(nav_cluster_keys), nav_pos, key_unit, scale, flags);

		// Numeric keypad
		if (!(flags & ImGuiKeyboardFlags_NoNumpad)) {
			float numpad_x = nav_x + 3.0f * key_unit + section_gap;
			ImVec2 numpad_pos = ImVec2(numpad_x, main_section_y);
			RenderKeyRow(draw_list, numpad_keys, IM_ARRAYSIZE(numpad_keys), numpad_pos, key_unit, scale, flags);
		}

		// Handle recording for all key rows when Recordable flag is set
		if (mouse_clicked) {
			// Function row
			HandleKeyRowRecording(function_row_keys, IM_ARRAYSIZE(function_row_keys), func_row_pos, key_unit, scale, mouse_pos, mouse_clicked);
			HandleKeyRowRecording(function_row_nav_keys, IM_ARRAYSIZE(function_row_nav_keys), func_row_nav_pos, key_unit, scale, mouse_pos, mouse_clicked);
			// Number row
			HandleKeyRowRecording(num_row_keys, num_row_count, num_row_pos, key_unit, scale, mouse_pos, mouse_clicked);
			// Letter rows
			HandleKeyRowRecording(row1_keys, row1_count, row1_pos, key_unit, scale, mouse_pos, mouse_clicked);
			HandleKeyRowRecording(row2_keys, row2_count, row2_pos, key_unit, scale, mouse_pos, mouse_clicked);
			// ISO Enter key (if ISO layout)
			if (isISOLayout && IsMouseInISOEnterKey(mouse_pos, row1_pos, row2_pos, key_unit)) {
				Record(ImGuiKey_Enter, !IsKeyRecorded(ImGuiKey_Enter));
			}
			HandleKeyRowRecording(row3_keys, row3_count, row3_pos, key_unit, scale, mouse_pos, mouse_clicked);
			// Bottom row
			if (isAppleLayout) {
				HandleKeyRowRecording(apple_bottom_row_keys, IM_ARRAYSIZE(apple_bottom_row_keys), bottom_row_pos, key_unit, scale, mouse_pos, mouse_clicked);
			} else {
				HandleKeyRowRecording(bottom_row_keys, IM_ARRAYSIZE(bottom_row_keys), bottom_row_pos, key_unit, scale, mouse_pos, mouse_clicked);
			}
			// Navigation cluster
			HandleKeyRowRecording(nav_cluster_keys, IM_ARRAYSIZE(nav_cluster_keys), nav_pos, key_unit, scale, mouse_pos, mouse_clicked);
			// Numpad (if visible)
			if (!(flags & ImGuiKeyboardFlags_NoNumpad)) {
				float numpad_x_recording = nav_x + 3.0f * key_unit + section_gap;
				ImVec2 numpad_pos_recording = ImVec2(numpad_x_recording, main_section_y);
				HandleKeyRowRecording(numpad_keys, IM_ARRAYSIZE(numpad_keys), numpad_pos_recording, key_unit, scale, mouse_pos, mouse_clicked);
			}
		}
	}

	draw_list->PopClipRect();
}

#ifndef IMGUI_DISABLE_DEMO_WINDOWS
void KeyboardDemo() {
	static bool showPressed = true;
	static bool noShiftLabels = false;
	static bool showBothLabels = false;
	static bool showIcons = false;
	static bool noNumpad = false;
	static bool recordable = false;
	static int currentLayout = ImGuiKeyboardLayout_Qwerty;
	static bool highlightWASD = false;
	static bool highlightArrows = false;
	static bool highlightNumpad = false;
	static bool showStyleEditor = false;

	ImGui::Text("Keyboard Widget Demo");
	ImGui::Separator();

	// Layout selection
	ImGui::Text("Layout:");
	ImGui::SameLine();
	const char *layoutNames[] = {"QWERTY (ANSI)", "QWERTZ (ANSI)", "AZERTY (ANSI)", "Colemak", "Dvorak", "Numeric Pad", "QWERTY (ISO)", "QWERTZ (ISO)", "AZERTY (ISO)", "Apple (ANSI)", "Apple (ISO)"};
	if (ImGui::BeginCombo("##Layout", layoutNames[currentLayout])) {
		for (int i = 0; i < ImGuiKeyboardLayout_Count; i++) {
			const bool isSelected = (currentLayout == i);
			if (ImGui::Selectable(layoutNames[i], isSelected)) {
				currentLayout = i;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	// Flags
	ImGui::Text("Options:");
	ImGui::Checkbox("Show Pressed Keys (Red)", &showPressed);
	if (ImGui::Checkbox("Show Both Labels (Shift + Normal)", &showBothLabels)) {
		if (showBothLabels) {
			noShiftLabels = false; // Disable conflicting option
		}
	}
	if (ImGui::Checkbox("Disable Shift Labels", &noShiftLabels)) {
		if (noShiftLabels) {
			showBothLabels = false; // Disable conflicting option
		}
	}
	ImGui::Checkbox("Show Icons", &showIcons);
	if (currentLayout != ImGuiKeyboardLayout_NumericPad) {
		ImGui::Checkbox("Hide Numpad", &noNumpad);
	}
	ImGui::Checkbox("Recordable Keys (Blue)", &recordable);

	// Show recorded keys when recordable mode is enabled
	if (recordable) {
		const ImVector<ImGuiKey>& recordedKeys = GetRecordedKeys();
		if (recordedKeys.Size > 0) {
			ImGui::Text("Recorded Keys (%d):", recordedKeys.Size);
			ImGui::SameLine();
			for (int i = 0; i < recordedKeys.Size; i++) {
				if (i > 0) ImGui::SameLine();
				ImGui::Text("%s%s", ImGui::GetKeyName(recordedKeys[i]), i < recordedKeys.Size - 1 ? "," : "");
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Clear")) {
				ClearRecorded();
			}
		} else {
			ImGui::Text("Click or press keys to record them");
		}
	}

	ImGui::Separator();
	ImGui::Text("Highlight Groups (Green):");

	// WASD highlight toggle
	if (ImGui::Checkbox("Highlight WASD", &highlightWASD)) {
		Highlight(ImGuiKey_W, highlightWASD);
		Highlight(ImGuiKey_A, highlightWASD);
		Highlight(ImGuiKey_S, highlightWASD);
		Highlight(ImGuiKey_D, highlightWASD);
	}

	// Arrow keys highlight toggle
	ImGui::SameLine();
	if (ImGui::Checkbox("Highlight Arrows", &highlightArrows)) {
		Highlight(ImGuiKey_UpArrow, highlightArrows);
		Highlight(ImGuiKey_DownArrow, highlightArrows);
		Highlight(ImGuiKey_LeftArrow, highlightArrows);
		Highlight(ImGuiKey_RightArrow, highlightArrows);
	}

	// Numpad highlight toggle
	ImGui::SameLine();
	if (ImGui::Checkbox("Highlight Numpad", &highlightNumpad)) {
		Highlight(ImGuiKey_Keypad0, highlightNumpad);
		Highlight(ImGuiKey_Keypad1, highlightNumpad);
		Highlight(ImGuiKey_Keypad2, highlightNumpad);
		Highlight(ImGuiKey_Keypad3, highlightNumpad);
		Highlight(ImGuiKey_Keypad4, highlightNumpad);
		Highlight(ImGuiKey_Keypad5, highlightNumpad);
		Highlight(ImGuiKey_Keypad6, highlightNumpad);
		Highlight(ImGuiKey_Keypad7, highlightNumpad);
		Highlight(ImGuiKey_Keypad8, highlightNumpad);
		Highlight(ImGuiKey_Keypad9, highlightNumpad);
	}

	// Individual key highlight
	ImGui::Text("Highlight Individual Key:");
	static int selectedKey = 0;
	static bool keyHighlighted = false;
	const char *keyNames[] = {"Space", "Enter", "Tab", "Escape", "Backspace", "Left Shift", "Left Ctrl", "Left Alt"};
	const ImGuiKey keyValues[] = {ImGuiKey_Space,	  ImGuiKey_Enter,	  ImGuiKey_Tab,		 ImGuiKey_Escape,
								  ImGuiKey_Backspace, ImGuiKey_LeftShift, ImGuiKey_LeftCtrl, ImGuiKey_LeftAlt};
	ImGui::SetNextItemWidth(150);
	if (ImGui::BeginCombo("##Key", keyNames[selectedKey])) {
		for (int i = 0; i < IM_ARRAYSIZE(keyNames); i++) {
			const bool isSelected = (selectedKey == i);
			if (ImGui::Selectable(keyNames[i], isSelected)) {
				// Remove highlight from previous key
				if (keyHighlighted) {
					Highlight(keyValues[selectedKey], false);
				}
				selectedKey = i;
				// Apply highlight to new key if checkbox is checked
				if (keyHighlighted) {
					Highlight(keyValues[selectedKey], true);
				}
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	if (ImGui::Checkbox("Highlight##IndividualKey", &keyHighlighted)) {
		Highlight(keyValues[selectedKey], keyHighlighted);
	}

	// Clear all highlights button
	if (ImGui::Button("Clear All Highlights")) {
		ClearHighlights();
		highlightWASD = false;
		highlightArrows = false;
		highlightNumpad = false;
		keyHighlighted = false;
	}

	ImGui::Separator();

	// Style editor toggle
	ImGui::Checkbox("Show Style Editor", &showStyleEditor);
	if (showStyleEditor) {
		ImGuiKeyboardStyle &style = GetStyle();

		if (ImGui::TreeNode("Sizes")) {
			ImGui::SliderFloat("Key Unit", &style.KeyUnit, 20.0f, 60.0f, "%.1f");
			ImGui::SliderFloat("Section Gap", &style.SectionGap, 0.0f, 30.0f, "%.1f");
			ImGui::SliderFloat("Key Border Size", &style.KeyBorderSize, 0.0f, 10.0f, "%.1f");
			ImGui::SliderFloat("Key Rounding", &style.KeyRounding, 0.0f, 10.0f, "%.1f");
			ImGui::SliderFloat("Key Face Rounding", &style.KeyFaceRounding, 0.0f, 10.0f, "%.1f");
			ImGui::SliderFloat("Key Face Border", &style.KeyFaceBorderSize, 0.0f, 5.0f, "%.1f");
			ImGui::SliderFloat2("Key Face Offset", &style.KeyFaceOffset.x, 0.0f, 10.0f, "%.1f");
			ImGui::SliderFloat2("Key Label Offset", &style.KeyLabelOffset.x, 0.0f, 15.0f, "%.1f");
			ImGui::SliderFloat("Board Padding", &style.BoardPadding, 0.0f, 20.0f, "%.1f");
			ImGui::SliderFloat("Board Rounding", &style.BoardRounding, 0.0f, 20.0f, "%.1f");

			if (ImGui::Button("Reset Sizes")) {
				ImGuiKeyboardStyle defaultStyle;
				style.KeyUnit = defaultStyle.KeyUnit;
				style.SectionGap = defaultStyle.SectionGap;
				style.KeyBorderSize = defaultStyle.KeyBorderSize;
				style.KeyRounding = defaultStyle.KeyRounding;
				style.KeyFaceRounding = defaultStyle.KeyFaceRounding;
				style.KeyFaceBorderSize = defaultStyle.KeyFaceBorderSize;
				style.KeyFaceOffset = defaultStyle.KeyFaceOffset;
				style.KeyLabelOffset = defaultStyle.KeyLabelOffset;
				style.BoardPadding = defaultStyle.BoardPadding;
				style.BoardRounding = defaultStyle.BoardRounding;
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Colors")) {
			const char *colorNames[] = {"Board Background", "Key Background",  "Key Border",
										"Key Face Border",	"Key Face",		   "Key Label",
										"Key Pressed",		"Key Highlighted", "Key Pressed+Highlighted",
										"Key Recorded"};
			for (int i = 0; i < ImGuiKeyboardCol_COUNT; i++) {
				ImGui::ColorEdit4(colorNames[i], &style.Colors[i].x,
								  ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf);
			}

			if (ImGui::Button("Reset Colors")) {
				ImGuiKeyboardStyle defaultStyle;
				for (int i = 0; i < ImGuiKeyboardCol_COUNT; i++) {
					style.Colors[i] = defaultStyle.Colors[i];
				}
			}
			ImGui::TreePop();
		}
	}

	ImGui::Text("Legend: (Default) Red = Pressed | Green = Highlighted | Yellow = Both | Blue = Recorded");
	ImGui::Separator();

	// Render the keyboard
	ImGuiKeyboardFlags flags = ImGuiKeyboardFlags_None;
	if (showPressed) {
		flags |= ImGuiKeyboardFlags_ShowPressed;
	}
	if (noShiftLabels) {
		flags |= ImGuiKeyboardFlags_NoShiftLabels;
	}
	if (showBothLabels) {
		flags |= ImGuiKeyboardFlags_ShowBothLabels;
	}
	if (showIcons) {
		flags |= ImGuiKeyboardFlags_ShowIcons;
	}
	if (noNumpad && currentLayout != ImGuiKeyboardLayout_NumericPad) {
		flags |= ImGuiKeyboardFlags_NoNumpad;
	}
	if (recordable) {
		flags |= ImGuiKeyboardFlags_Recordable;
	}
	Keyboard((ImGuiKeyboardLayout)currentLayout, flags);
}
#endif // IMGUI_DISABLE_DEMO_WINDOWS
#endif // IMGUI_DISABLE

} // namespace ImKeyboard
