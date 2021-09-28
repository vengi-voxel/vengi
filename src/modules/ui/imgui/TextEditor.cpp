#include "TextEditor.h"
#include "core/Common.h"
#include "core/UTF8.h"
#include "dearimgui/imgui.h"
#include "glm/common.hpp"
#include <algorithm>

#include <SDL.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "IMGUI.h" // for imGui::GetCurrentWindow()

template <class InputIt1, class InputIt2, class BinaryPredicate>
bool equals(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, BinaryPredicate p) {
	for (; first1 != last1 && first2 != last2; ++first1, ++first2) {
		if (!p(*first1, *first2))
			return false;
	}
	return first1 == last1 && first2 == last2;
}

TextEditor::TextEditor() : _startTime(SDL_GetTicks()) {
	SetPalette(GetDarkPalette());
	SetLanguageDefinition(LanguageDefinition::Lua());
	_lines.push_back(Line());
}

TextEditor::~TextEditor() {
}

void TextEditor::SetLanguageDefinition(const LanguageDefinition &aLanguageDef) {
	_languageDefinition = aLanguageDef;
	_regexList.clear();

	for (const auto &r : _languageDefinition.mTokenRegexStrings) {
		_regexList.push_back(std::make_pair(std::regex(r.first.c_str(), std::regex_constants::optimize), r.second));
	}

	Colorize();
}

void TextEditor::SetPalette(const Palette &aValue) {
	_paletteBase = aValue;
}

core::String TextEditor::GetText(const Coordinates &aStart, const Coordinates &aEnd) const {
	core::String result;

	int lstart = aStart.mLine;
	int lend = aEnd.mLine;
	int istart = GetCharacterIndex(aStart);
	int iend = GetCharacterIndex(aEnd);
	size_t s = 0;

	for (int i = lstart; i < lend; i++)
		s += _lines[i].size();

	result.reserve(s + s / 8);

	while (istart < iend || lstart < lend) {
		if (lstart >= (int)_lines.size())
			break;

		auto &line = _lines[lstart];
		if (istart < (int)line.size()) {
			result += line[istart].mChar;
			istart++;
		} else {
			istart = 0;
			++lstart;
			result += '\n';
		}
	}

	return result;
}

TextEditor::Coordinates TextEditor::GetActualCursorCoordinates() const {
	return SanitizeCoordinates(_state.mCursorPosition);
}

TextEditor::Coordinates TextEditor::SanitizeCoordinates(const Coordinates &aValue) const {
	auto line = aValue.mLine;
	auto column = aValue.mColumn;
	if (line >= (int)_lines.size()) {
		if (_lines.empty()) {
			line = 0;
			column = 0;
		} else {
			line = (int)_lines.size() - 1;
			column = GetLineMaxColumn(line);
		}
		return Coordinates(line, column);
	} else {
		column = _lines.empty() ? 0 : core_min(column, GetLineMaxColumn(line));
		return Coordinates(line, column);
	}
}

void TextEditor::Advance(Coordinates &aCoordinates) const {
	if (aCoordinates.mLine < (int)_lines.size()) {
		auto &line = _lines[aCoordinates.mLine];
		auto cindex = GetCharacterIndex(aCoordinates);

		if (cindex + 1 < (int)line.size()) {
			const size_t delta = core::utf8::lengthInt(line[cindex].mChar);
			cindex = core_min(cindex + (int)delta, (int)line.size() - 1);
		} else {
			++aCoordinates.mLine;
			cindex = 0;
		}
		aCoordinates.mColumn = GetCharacterColumn(aCoordinates.mLine, cindex);
	}
}

void TextEditor::DeleteRange(const Coordinates &aStart, const Coordinates &aEnd) {
	core_assert(aEnd >= aStart);
	core_assert(!_readOnly);

	// printf("D(%d.%d)-(%d.%d)\n", aStart.mLine, aStart.mColumn, aEnd.mLine, aEnd.mColumn);

	if (aEnd == aStart)
		return;

	auto start = GetCharacterIndex(aStart);
	auto end = GetCharacterIndex(aEnd);

	if (aStart.mLine == aEnd.mLine) {
		auto &line = _lines[aStart.mLine];
		auto n = GetLineMaxColumn(aStart.mLine);
		if (aEnd.mColumn >= n)
			line.erase(start, line.size());
		else
			line.erase(start, end);
	} else {
		auto &firstLine = _lines[aStart.mLine];
		auto &lastLine = _lines[aEnd.mLine];

		firstLine.erase(start, firstLine.size());
		lastLine.erase(0, end);

		if (aStart.mLine < aEnd.mLine)
			firstLine.insert(firstLine.end(), lastLine.begin(), lastLine.end());

		if (aStart.mLine < aEnd.mLine)
			RemoveLine(aStart.mLine + 1, aEnd.mLine + 1);
	}

	_textChanged = true;
}

int TextEditor::InsertTextAt(Coordinates & /* inout */ aWhere, const char *aValue) {
	core_assert(!_readOnly);

	int cindex = GetCharacterIndex(aWhere);
	int totalLines = 0;
	while (*aValue != '\0') {
		core_assert(!_lines.empty());

		if (*aValue == '\r') {
			// skip
			++aValue;
		} else if (*aValue == '\n') {
			if (cindex < (int)_lines[aWhere.mLine].size()) {
				auto &newLine = InsertLine(aWhere.mLine + 1);
				auto &line = _lines[aWhere.mLine];
				newLine.insert(newLine.begin(), line.begin() + cindex, line.end());
				line.erase(cindex, line.size());
			} else {
				InsertLine(aWhere.mLine + 1);
			}
			++aWhere.mLine;
			aWhere.mColumn = 0;
			cindex = 0;
			++totalLines;
			++aValue;
		} else {
			auto &line = _lines[aWhere.mLine];
			auto d = core::utf8::lengthInt((int)*aValue);
			while (d-- > 0 && *aValue != '\0')
				line.insert(line.begin() + cindex++, Glyph(*aValue++, PaletteIndex::Default));
			++aWhere.mColumn;
		}

		_textChanged = true;
	}

	return totalLines;
}

void TextEditor::AddUndo(UndoRecord &aValue) {
	core_assert(!_readOnly);
	// printf("AddUndo: (@%d.%d) +\'%s' [%d.%d .. %d.%d], -\'%s', [%d.%d .. %d.%d] (@%d.%d)\n",
	//	aValue.mBefore.mCursorPosition.mLine, aValue.mBefore.mCursorPosition.mColumn,
	//	aValue.mAdded.c_str(), aValue.mAddedStart.mLine, aValue.mAddedStart.mColumn, aValue.mAddedEnd.mLine,
	// aValue.mAddedEnd.mColumn, 	aValue.mRemoved.c_str(), aValue.mRemovedStart.mLine, aValue.mRemovedStart.mColumn,
	// aValue.mRemovedEnd.mLine, aValue.mRemovedEnd.mColumn, 	aValue.mAfter.mCursorPosition.mLine,
	// aValue.mAfter.mCursorPosition.mColumn
	//	);

	_undoBuffer.resize((size_t)(_undoIndex + 1));
	_undoBuffer.back() = aValue;
	++_undoIndex;
}

TextEditor::Coordinates TextEditor::ScreenPosToCoordinates(const ImVec2 &aPosition) const {
	ImVec2 origin = ImGui::GetCursorScreenPos();
	ImVec2 local(aPosition.x - origin.x, aPosition.y - origin.y);

	int lineNo = core_max(0, (int)glm::floor(local.y / _charAdvance.y));

	int columnCoord = 0;

	if (lineNo >= 0 && lineNo < (int)_lines.size()) {
		auto &line = _lines[lineNo];

		int columnIndex = 0;
		float columnX = 0.0f;

		while ((size_t)columnIndex < line.size()) {
			float columnWidth = 0.0f;

			if (line[columnIndex].mChar == '\t') {
				float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ").x;
				float oldX = columnX;
				float newColumnX = (1.0f + glm::floor((1.0f + columnX) / (float(_tabSize) * spaceSize))) *
								   (float(_tabSize) * spaceSize);
				columnWidth = newColumnX - oldX;
				if (_textStart + columnX + columnWidth * 0.5f > local.x)
					break;
				columnX = newColumnX;
				columnCoord = (columnCoord / _tabSize) * _tabSize + _tabSize;
				columnIndex++;
			} else {
				char buf[7];
				auto d = core::utf8::lengthInt((int)line[columnIndex].mChar);
				int i = 0;
				while (i < 6 && d-- > 0)
					buf[i++] = line[columnIndex++].mChar;
				buf[i] = '\0';
				columnWidth = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf).x;
				if (_textStart + columnX + columnWidth * 0.5f > local.x)
					break;
				columnX += columnWidth;
				columnCoord++;
			}
		}
	}

	return SanitizeCoordinates(Coordinates(lineNo, columnCoord));
}

TextEditor::Coordinates TextEditor::FindWordStart(const Coordinates &aFrom) const {
	Coordinates at = aFrom;
	if (at.mLine >= (int)_lines.size()) {
		return at;
	}

	const Line &line = _lines[at.mLine];
	int cindex = GetCharacterIndex(at);

	if (cindex >= (int)line.size()) {
		return at;
	}

	while (cindex > 0 && SDL_isspace(line[cindex].mChar)) {
		--cindex;
	}

	PaletteIndex cstart = (PaletteIndex)line[cindex].mColorIndex;
	while (cindex > 0) {
		Char c = line[cindex].mChar;
		if (!core::utf8::isMultibyte(c)) {
			if (c <= 32 && SDL_isspace(c)) {
				++cindex;
				break;
			}
			if (cstart != (PaletteIndex)line[size_t(cindex - 1)].mColorIndex) {
				break;
			}
		}
		--cindex;
	}
	return Coordinates(at.mLine, GetCharacterColumn(at.mLine, cindex));
}

TextEditor::Coordinates TextEditor::FindWordEnd(const Coordinates &aFrom) const {
	Coordinates at = aFrom;
	if (at.mLine >= (int)_lines.size()) {
		return at;
	}

	const Line &line = _lines[at.mLine];
	int cindex = GetCharacterIndex(at);

	if (cindex >= (int)line.size()) {
		return at;
	}

	bool prevspace = (bool)SDL_isspace(line[cindex].mChar);
	PaletteIndex cstart = (PaletteIndex)line[cindex].mColorIndex;
	while (cindex < (int)line.size()) {
		Char c = line[cindex].mChar;
		size_t d = core::utf8::lengthInt((int)c);
		if (cstart != (PaletteIndex)line[cindex].mColorIndex)
			break;

		if (prevspace != !!SDL_isspace(c)) {
			if (SDL_isspace(c))
				while (cindex < (int)line.size() && SDL_isspace(line[cindex].mChar))
					++cindex;
			break;
		}
		cindex += d;
	}
	return Coordinates(aFrom.mLine, GetCharacterColumn(aFrom.mLine, cindex));
}

TextEditor::Coordinates TextEditor::FindNextWord(const Coordinates &aFrom) const {
	Coordinates at = aFrom;
	if (at.mLine >= (int)_lines.size()) {
		return at;
	}

	// skip to the next non-word character
	auto cindex = GetCharacterIndex(aFrom);
	bool isword = false;
	bool skip = false;
	if (cindex < (int)_lines[at.mLine].size()) {
		auto &line = _lines[at.mLine];
		isword = isalnum(line[cindex].mChar);
		skip = isword;
	}

	while (!isword || skip) {
		if (at.mLine >= (int)_lines.size()) {
			auto l = core_max(0, (int)_lines.size() - 1);
			return Coordinates(l, GetLineMaxColumn(l));
		}

		auto &line = _lines[at.mLine];
		if (cindex < (int)line.size()) {
			isword = isalnum(line[cindex].mChar);

			if (isword && !skip)
				return Coordinates(at.mLine, GetCharacterColumn(at.mLine, cindex));

			if (!isword)
				skip = false;

			cindex++;
		} else {
			cindex = 0;
			++at.mLine;
			skip = false;
			isword = false;
		}
	}

	return at;
}

int TextEditor::GetCharacterIndex(const Coordinates &aCoordinates) const {
	if (aCoordinates.mLine >= (int)_lines.size()) {
		return -1;
	}
	auto &line = _lines[aCoordinates.mLine];
	int c = 0;
	int i = 0;
	for (; i < (int)line.size() && c < aCoordinates.mColumn;) {
		if (line[i].mChar == '\t') {
			c = (c / _tabSize) * _tabSize + _tabSize;
		} else {
			++c;
		}
		i += core::utf8::lengthInt((int)line[i].mChar);
	}
	return i;
}

int TextEditor::GetCharacterColumn(int aLine, int aIndex) const {
	if (aLine >= (int)_lines.size()) {
		return 0;
	}
	const Line &line = _lines[aLine];
	int col = 0;
	int i = 0;
	while (i < aIndex && i < (int)line.size()) {
		Char c = line[i].mChar;
		i += core::utf8::lengthInt((int)c);
		if (c == '\t') {
			col = (col / _tabSize) * _tabSize + _tabSize;
		} else {
			++col;
		}
	}
	return col;
}

int TextEditor::GetLineCharacterCount(int aLine) const {
	if (aLine >= (int)_lines.size()) {
		return 0;
	}
	auto &line = _lines[aLine];
	int c = 0;
	for (size_t i = 0; i < line.size(); ++c) {
		i += core::utf8::lengthInt((int)line[i].mChar);
	}
	return c;
}

int TextEditor::GetLineMaxColumn(int aLine) const {
	if (aLine >= (int)_lines.size()) {
		return 0;
	}
	auto &line = _lines[aLine];
	int col = 0;
	for (unsigned i = 0; i < line.size();) {
		auto c = line[i].mChar;
		if (c == '\t') {
			col = (col / _tabSize) * _tabSize + _tabSize;
		} else {
			++col;
		}
		i += core::utf8::lengthInt((int)c);
	}
	return col;
}

bool TextEditor::IsOnWordBoundary(const Coordinates &aAt) const {
	if (aAt.mLine >= (int)_lines.size() || aAt.mColumn == 0) {
		return true;
	}

	const Line &line = _lines[aAt.mLine];
	int cindex = GetCharacterIndex(aAt);
	if (cindex >= (int)line.size()) {
		return true;
	}

	if (_colorizerEnabled) {
		return line[cindex].mColorIndex != line[size_t(cindex - 1)].mColorIndex;
	}

	return SDL_isspace(line[cindex].mChar) != SDL_isspace(line[cindex - 1].mChar);
}

void TextEditor::RemoveLine(int aStart, int aEnd) {
	core_assert(!_readOnly);
	core_assert(aEnd >= aStart);
	core_assert(_lines.size() > (size_t)(aEnd - aStart));

	ErrorMarkers etmp;
	for (const auto &i : _errorMarkers) {
		int line = i->first >= aStart ? i->first - 1 : i->first;
		if (line >= aStart && line <= aEnd) {
			continue;
		}
		etmp.put(line, i->second);
	}
	_errorMarkers = core::move(etmp);

	Breakpoints btmp;
	for (auto i : _breakpoints) {
		if (i->second >= aStart && i->second <= aEnd) {
			continue;
		}
		btmp.insert(i->second >= aStart ? i->second - 1 : i->second);
	}
	_breakpoints = core::move(btmp);

	_lines.erase(aStart, aEnd);
	core_assert(!_lines.empty());

	_textChanged = true;
}

void TextEditor::RemoveLine(int aIndex) {
	core_assert(!_readOnly);
	core_assert(_lines.size() > 1);

	ErrorMarkers etmp;
	for (const auto &i : _errorMarkers) {
		int line = i->first >= aIndex ? i->first - 1 : i->first;
		if (line - 1 == aIndex) {
			continue;
		}
		etmp.put(line, i->second);
	}
	_errorMarkers = core::move(etmp);

	Breakpoints btmp;
	for (auto i : _breakpoints) {
		if (i->second == aIndex) {
			continue;
		}
		btmp.insert(i->second >= aIndex ? i->second - 1 : i->second);
	}
	_breakpoints = core::move(btmp);

	_lines.erase(_lines.begin() + aIndex);
	core_assert(!_lines.empty());

	_textChanged = true;
}

TextEditor::Line &TextEditor::InsertLine(int aIndex) {
	core_assert(!_readOnly);

	_lines.insert(_lines.begin() + aIndex, Line());
	auto &result = _lines[aIndex];

	ErrorMarkers etmp;
	for (const auto &i : _errorMarkers) {
		etmp.put(i->first >= aIndex ? i->first + 1 : i->first, i->second);
	}
	_errorMarkers = core::move(etmp);

	Breakpoints btmp;
	for (auto i : _breakpoints) {
		btmp.insert(i->second >= aIndex ? i->second + 1 : i->second);
	}
	_breakpoints = core::move(btmp);

	return result;
}

core::String TextEditor::GetWordUnderCursor() const {
	const Coordinates &c = GetCursorPosition();
	return GetWordAt(c);
}

core::String TextEditor::GetWordAt(const Coordinates &aCoords) const {
	const Coordinates &start = FindWordStart(aCoords);
	const Coordinates &end = FindWordEnd(aCoords);

	int istart = GetCharacterIndex(start);
	int iend = GetCharacterIndex(end);

	core::String r;
	r.reserve(iend - istart + 1);
	for (int it = istart; it < iend; ++it) {
		r += _lines[aCoords.mLine][it].mChar;
	}

	return r;
}

ImU32 TextEditor::GetGlyphColor(const Glyph &aGlyph) const {
	if (!_colorizerEnabled) {
		return _palette[(int)PaletteIndex::Default];
	}
	if (aGlyph.mComment) {
		return _palette[(int)PaletteIndex::Comment];
	}
	if (aGlyph.mMultiLineComment) {
		return _palette[(int)PaletteIndex::MultiLineComment];
	}
	const ImU32 color = _palette[(int)aGlyph.mColorIndex];
	if (aGlyph.mPreprocessor) {
		const auto ppcolor = _palette[(int)PaletteIndex::Preprocessor];
		const int c0 = (int)((ppcolor & 0xff) + (color & 0xff)) / 2;
		const int c1 = (int)(((ppcolor >> 8) & 0xff) + ((color >> 8) & 0xff)) / 2;
		const int c2 = (int)(((ppcolor >> 16) & 0xff) + ((color >> 16) & 0xff)) / 2;
		const int c3 = (int)(((ppcolor >> 24) & 0xff) + ((color >> 24) & 0xff)) / 2;
		return ImU32(c0 | (c1 << 8) | (c2 << 16) | (c3 << 24));
	}
	return color;
}

void TextEditor::HandleKeyboardInputs() {
	ImGuiIO &io = ImGui::GetIO();
	bool shift = io.KeyShift;
	bool ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	bool alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

	if (ImGui::IsWindowFocused()) {
		if (ImGui::IsWindowHovered()) {
			ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
		}
		// ImGui::CaptureKeyboardFromApp(true);

		io.WantCaptureKeyboard = true;
		io.WantTextInput = true;

		if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z)))
			Undo();
		else if (!IsReadOnly() && !ctrl && !shift && alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
			Undo();
		else if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y)))
			Redo();
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))
			MoveUp(1, shift);
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
			MoveDown(1, shift);
		else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)))
			MoveLeft(1, shift, ctrl);
		else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)))
			MoveRight(1, shift, ctrl);
		else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_PageUp)))
			MoveUp(GetPageSize() - 4, shift);
		else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_PageDown)))
			MoveDown(GetPageSize() - 4, shift);
		else if (!alt && ctrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Home)))
			MoveTop(shift);
		else if (ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_End)))
			MoveBottom(shift);
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Home)))
			MoveHome(shift);
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_End)))
			MoveEnd(shift);
		else if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)))
			Delete();
		else if (!IsReadOnly() && !ctrl && !shift && !alt &&
				 ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
			Backspace();
		else if (!ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
			_overwrite ^= true;
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
			Copy();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C)))
			Copy();
		else if (!IsReadOnly() && !ctrl && shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
			Paste();
		else if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_V)))
			Paste();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_X)))
			Cut();
		else if (!ctrl && shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)))
			Cut();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A)))
			SelectAll();
		else if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
			EnterCharacter('\n', false);
		else if (!IsReadOnly() && !ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Tab)))
			EnterCharacter('\t', shift);

		if (!IsReadOnly() && !io.InputQueueCharacters.empty()) {
			for (int i = 0; i < io.InputQueueCharacters.Size; i++) {
				auto c = io.InputQueueCharacters[i];
				if (c != 0 && (c == '\n' || c >= 32))
					EnterCharacter(c, shift);
			}
			io.InputQueueCharacters.resize(0);
		}
	}
}

void TextEditor::HandleMouseInputs() {
	ImGuiIO &io = ImGui::GetIO();
	bool shift = io.KeyShift;
	bool ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	bool alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

	if (ImGui::IsWindowHovered()) {
		if (!shift && !alt) {
			bool click = ImGui::IsMouseClicked(0);
			bool doubleClick = ImGui::IsMouseDoubleClicked(0);
			double t = ImGui::GetTime();
			bool tripleClick =
				click && !doubleClick && (_lastClick != -1.0f && (t - _lastClick) < io.MouseDoubleClickTime);

			if (tripleClick) {
				if (!ctrl) {
					_state.mCursorPosition = _interactiveStart = _interactiveEnd =
						ScreenPosToCoordinates(ImGui::GetMousePos());
					_selectionMode = SelectionMode::Line;
					SetSelection(_interactiveStart, _interactiveEnd, _selectionMode);
				}

				_lastClick = -1.0f;
			} else if (doubleClick) {
				if (!ctrl) {
					_state.mCursorPosition = _interactiveStart = _interactiveEnd =
						ScreenPosToCoordinates(ImGui::GetMousePos());
					if (_selectionMode == SelectionMode::Line)
						_selectionMode = SelectionMode::Normal;
					else
						_selectionMode = SelectionMode::Word;
					SetSelection(_interactiveStart, _interactiveEnd, _selectionMode);
				}

				_lastClick = (float)ImGui::GetTime();
			} else if (click) {
				_state.mCursorPosition = _interactiveStart = _interactiveEnd =
					ScreenPosToCoordinates(ImGui::GetMousePos());
				if (ctrl)
					_selectionMode = SelectionMode::Word;
				else
					_selectionMode = SelectionMode::Normal;
				SetSelection(_interactiveStart, _interactiveEnd, _selectionMode);

				_lastClick = (float)ImGui::GetTime();
			}
			// Mouse left button dragging (=> update selection)
			else if (ImGui::IsMouseDragging(0) && ImGui::IsMouseDown(0)) {
				io.WantCaptureMouse = true;
				_state.mCursorPosition = _interactiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
				SetSelection(_interactiveStart, _interactiveEnd, _selectionMode);
			}
		}
	}
}

void TextEditor::Render() {
	/* Compute mCharAdvance regarding to scaled font size (Ctrl + mouse wheel)*/
	const float fontSize =
		ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr).x;
	_charAdvance = ImVec2(fontSize, ImGui::GetTextLineHeightWithSpacing() * _lineSpacing);

	/* Update palette with the current alpha from style */
	for (int i = 0; i < (int)PaletteIndex::Max; ++i) {
		auto color = ImGui::ColorConvertU32ToFloat4(_paletteBase[i]);
		color.w *= ImGui::GetStyle().Alpha;
		_palette[i] = ImGui::ColorConvertFloat4ToU32(color);
	}

	core_assert(_lineBuffer.empty());

	const ImVec2 &contentSize = ImGui::GetWindowContentRegionMax();
	ImDrawList *drawList = ImGui::GetWindowDrawList();
	float longest(_textStart);

	if (_scrollToTop) {
		_scrollToTop = false;
		ImGui::SetScrollY(0.f);
	}

	ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
	float scrollX = ImGui::GetScrollX();
	float scrollY = ImGui::GetScrollY();

	auto lineNo = (int)glm::floor(scrollY / _charAdvance.y);
	auto globalLineMax = (int)_lines.size();
	auto lineMax = core_max(
		0, core_min((int)_lines.size() - 1, lineNo + (int)glm::floor((scrollY + contentSize.y) / _charAdvance.y)));

	// Deduce _textStart by evaluating _lines size (global lineMax) plus two spaces as text width
	char buf[16];
	snprintf(buf, 16, " %d ", globalLineMax);
	_textStart = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf, nullptr, nullptr).x +
				 (float)_leftMargin;

	if (!_lines.empty()) {
		float spaceSize =
			ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;

		while (lineNo <= lineMax) {
			ImVec2 lineStartScreenPos = ImVec2(cursorScreenPos.x, cursorScreenPos.y + lineNo * _charAdvance.y);
			ImVec2 textScreenPos = ImVec2(lineStartScreenPos.x + _textStart, lineStartScreenPos.y);

			Line &line = _lines[lineNo];
			longest =
				core_max(_textStart + TextDistanceToLineStart(Coordinates(lineNo, GetLineMaxColumn(lineNo))), longest);
			int columnNo = 0;
			Coordinates lineStartCoord(lineNo, 0);
			Coordinates lineEndCoord(lineNo, GetLineMaxColumn(lineNo));

			// Draw selection for the current line
			float sstart = -1.0f;
			float ssend = -1.0f;

			core_assert(_state.mSelectionStart <= _state.mSelectionEnd);
			if (_state.mSelectionStart <= lineEndCoord)
				sstart =
					_state.mSelectionStart > lineStartCoord ? TextDistanceToLineStart(_state.mSelectionStart) : 0.0f;
			if (_state.mSelectionEnd > lineStartCoord)
				ssend =
					TextDistanceToLineStart(_state.mSelectionEnd < lineEndCoord ? _state.mSelectionEnd : lineEndCoord);

			if (_state.mSelectionEnd.mLine > lineNo)
				ssend += _charAdvance.x;

			if (sstart != -1 && ssend != -1 && sstart < ssend) {
				const ImVec2 vstart(lineStartScreenPos.x + _textStart + sstart, lineStartScreenPos.y);
				const ImVec2 vend(lineStartScreenPos.x + _textStart + ssend, lineStartScreenPos.y + _charAdvance.y);
				drawList->AddRectFilled(vstart, vend, _palette[(int)PaletteIndex::Selection]);
			}

			// Draw breakpoints
			ImVec2 start(lineStartScreenPos.x + scrollX, lineStartScreenPos.y);

			if (_breakpoints.has(lineNo + 1)) {
				const ImVec2 end(lineStartScreenPos.x + contentSize.x + 2.0f * scrollX,
								 lineStartScreenPos.y + _charAdvance.y);
				drawList->AddRectFilled(start, end, _palette[(int)PaletteIndex::Breakpoint]);
			}

			// Draw error markers
			auto errorIt = _errorMarkers.find(lineNo + 1);
			if (errorIt != _errorMarkers.end()) {
				const ImVec2 end(lineStartScreenPos.x + contentSize.x + 2.0f * scrollX,
								 lineStartScreenPos.y + _charAdvance.y);
				drawList->AddRectFilled(start, end, _palette[(int)PaletteIndex::ErrorMarker]);

				if (ImGui::IsMouseHoveringRect(lineStartScreenPos, end)) {
					ImGui::BeginTooltip();
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
					ImGui::Text("Error at line %d:", errorIt->first);
					ImGui::PopStyleColor();
					ImGui::Separator();
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.2f, 1.0f));
					ImGui::Text("%s", errorIt->second.c_str());
					ImGui::PopStyleColor();
					ImGui::EndTooltip();
				}
			}

			// Draw line number (right aligned)
			SDL_snprintf(buf, 16, "%d  ", lineNo + 1);

			const float lineNoWidth =
				ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf, nullptr, nullptr).x;
			drawList->AddText(ImVec2(lineStartScreenPos.x + _textStart - lineNoWidth, lineStartScreenPos.y),
							  _palette[(int)PaletteIndex::LineNumber], buf);

			if (_state.mCursorPosition.mLine == lineNo) {
				bool focused = ImGui::IsWindowFocused();

				// Highlight the current line (where the cursor is)
				if (!HasSelection()) {
					const ImVec2 end(start.x + contentSize.x + scrollX, start.y + _charAdvance.y);
					drawList->AddRectFilled(start, end,
											_palette[(int)(focused ? PaletteIndex::CurrentLineFill
																   : PaletteIndex::CurrentLineFillInactive)]);
					drawList->AddRect(start, end, _palette[(int)PaletteIndex::CurrentLineEdge], 1.0f);
				}

				// Render the cursor
				if (focused) {
					uint64_t timeEnd = SDL_GetTicks();
					uint64_t elapsed = timeEnd - _startTime;
					if (elapsed > 400) {
						float width = 1.0f;
						int cindex = GetCharacterIndex(_state.mCursorPosition);
						float cx = TextDistanceToLineStart(_state.mCursorPosition);

						if (_overwrite && cindex < (int)line.size()) {
							Char c = line[cindex].mChar;
							if (c == '\t') {
								const float x = (1.0f + glm::floor((1.0f + cx) / (float(_tabSize) * spaceSize))) *
												(float(_tabSize) * spaceSize);
								width = x - cx;
							} else {
								char buf2[2];
								buf2[0] = line[cindex].mChar;
								buf2[1] = '\0';
								width = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf2).x;
							}
						}
						const ImVec2 cstart(textScreenPos.x + cx, lineStartScreenPos.y);
						const ImVec2 cend(textScreenPos.x + cx + width, lineStartScreenPos.y + _charAdvance.y);
						drawList->AddRectFilled(cstart, cend, _palette[(int)PaletteIndex::Cursor]);
						if (elapsed > 800)
							_startTime = timeEnd;
					}
				}
			}

			// Render colorized text
			ImU32 prevColor = line.empty() ? _palette[(int)PaletteIndex::Default] : GetGlyphColor(line[0]);
			ImVec2 bufferOffset;

			for (int i = 0; i < (int)line.size();) {
				Glyph &glyph = line[i];
				ImU32 color = GetGlyphColor(glyph);

				if ((color != prevColor || glyph.mChar == '\t' || glyph.mChar == ' ') && !_lineBuffer.empty()) {
					const ImVec2 newOffset(textScreenPos.x + bufferOffset.x, textScreenPos.y + bufferOffset.y);
					drawList->AddText(newOffset, prevColor, _lineBuffer.c_str());
					const ImVec2 &textSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f,
																			 _lineBuffer.c_str(), nullptr, nullptr);
					bufferOffset.x += textSize.x;
					_lineBuffer.clear();
				}
				prevColor = color;

				if (glyph.mChar == '\t') {
					float oldX = bufferOffset.x;
					bufferOffset.x = (1.0f + glm::floor((1.0f + bufferOffset.x) / (float(_tabSize) * spaceSize))) *
									 (float(_tabSize) * spaceSize);
					++i;

					if (_showWhitespaces) {
						const float s = ImGui::GetFontSize();
						const float x1 = textScreenPos.x + oldX + 1.0f;
						const float x2 = textScreenPos.x + bufferOffset.x - 1.0f;
						const float y = textScreenPos.y + bufferOffset.y + s * 0.5f;
						const ImVec2 p1(x1, y);
						const ImVec2 p2(x2, y);
						const ImVec2 p3(x2 - s * 0.2f, y - s * 0.2f);
						const ImVec2 p4(x2 - s * 0.2f, y + s * 0.2f);
						drawList->AddLine(p1, p2, 0x90909090);
						drawList->AddLine(p2, p3, 0x90909090);
						drawList->AddLine(p2, p4, 0x90909090);
					}
				} else if (glyph.mChar == ' ') {
					if (_showWhitespaces) {
						const float s = ImGui::GetFontSize();
						const float x = textScreenPos.x + bufferOffset.x + spaceSize * 0.5f;
						const float y = textScreenPos.y + bufferOffset.y + s * 0.5f;
						drawList->AddCircleFilled(ImVec2(x, y), 1.5f, 0x80808080, 4);
					}
					bufferOffset.x += spaceSize;
					i++;
				} else {
					int l = core::utf8::lengthInt((int)glyph.mChar);
					while (l-- > 0) {
						_lineBuffer += line[i++].mChar;
					}
				}
				++columnNo;
			}

			if (!_lineBuffer.empty()) {
				const ImVec2 newOffset(textScreenPos.x + bufferOffset.x, textScreenPos.y + bufferOffset.y);
				drawList->AddText(newOffset, prevColor, _lineBuffer.c_str());
				_lineBuffer.clear();
			}

			++lineNo;
		}

		// Draw a tooltip on known identifiers/preprocessor symbols
		if (ImGui::IsMousePosValid()) {
			const core::String &id = GetWordAt(ScreenPosToCoordinates(ImGui::GetMousePos()));
			if (!id.empty()) {
				auto it = _languageDefinition.mIdentifiers.find(id);
				if (it != _languageDefinition.mIdentifiers.end()) {
					ImGui::BeginTooltip();
					ImGui::TextUnformatted(it->second.mDeclaration.c_str());
					ImGui::EndTooltip();
				} else {
					auto pi = _languageDefinition.mPreprocIdentifiers.find(id);
					if (pi != _languageDefinition.mPreprocIdentifiers.end()) {
						ImGui::BeginTooltip();
						ImGui::TextUnformatted(pi->second.mDeclaration.c_str());
						ImGui::EndTooltip();
					}
				}
			}
		}
	}

	ImGui::Dummy(ImVec2((longest + 2), _lines.size() * _charAdvance.y));

	if (_scrollToCursor) {
		EnsureCursorVisible();
		ImGui::SetWindowFocus();
		_scrollToCursor = false;
	}
}

void TextEditor::Render(const char *aTitle, const ImVec2 &aSize, bool aBorder) {
	_withinRender = true;
	_textChanged = false;
	_cursorPositionChanged = false;

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
	if (!_ignoreImGuiChild)
		ImGui::BeginChild(aTitle, aSize, aBorder,
						  ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar |
							  ImGuiWindowFlags_NoMove);

	if (_handleKeyboardInputs) {
		HandleKeyboardInputs();
		ImGui::PushAllowKeyboardFocus(true);
	}

	if (_handleMouseInputs)
		HandleMouseInputs();

	ColorizeInternal();
	Render();

	if (_handleKeyboardInputs)
		ImGui::PopAllowKeyboardFocus();

	if (!_ignoreImGuiChild)
		ImGui::EndChild();

	ImGui::PopStyleVar();

	_withinRender = false;
}

void TextEditor::SetText(const core::String &aText) {
	_lines.clear();
	_lines.emplace_back(Line());
	for (char chr : aText) {
		if (chr == '\r') {
			// ignore the carriage return character
		} else if (chr == '\n') {
			_lines.emplace_back(Line());
		} else {
			_lines.back().emplace_back(Glyph(chr, PaletteIndex::Default));
		}
	}

	_textChanged = true;
	_scrollToTop = true;

	_undoBuffer.clear();
	_undoIndex = 0;

	Colorize();
}

void TextEditor::SetTextLines(const core::DynamicArray<core::String> &aLines) {
	_lines.clear();

	if (aLines.empty()) {
		_lines.emplace_back(Line());
	} else {
		_lines.resize(aLines.size());

		for (size_t i = 0; i < aLines.size(); ++i) {
			const core::String &aLine = aLines[i];

			_lines[i].reserve(aLine.size());
			for (size_t j = 0; j < aLine.size(); ++j)
				_lines[i].emplace_back(Glyph(aLine[j], PaletteIndex::Default));
		}
	}

	_textChanged = true;
	_scrollToTop = true;

	_undoBuffer.clear();
	_undoIndex = 0;

	Colorize();
}

void TextEditor::EnterCharacter(ImWchar aChar, bool aShift) {
	core_assert(!_readOnly);

	UndoRecord u;

	u.mBefore = _state;

	if (HasSelection()) {
		if (aChar == '\t' && _state.mSelectionStart.mLine != _state.mSelectionEnd.mLine) {

			auto start = _state.mSelectionStart;
			auto end = _state.mSelectionEnd;
			auto originalEnd = end;

			if (start > end)
				std::swap(start, end);
			start.mColumn = 0;
			//			end.mColumn = end.mLine < _lines.size() ? _lines[end.mLine].size() : 0;
			if (end.mColumn == 0 && end.mLine > 0)
				--end.mLine;
			if (end.mLine >= (int)_lines.size())
				end.mLine = _lines.empty() ? 0 : (int)_lines.size() - 1;
			end.mColumn = GetLineMaxColumn(end.mLine);

			// if (end.mColumn >= GetLineMaxColumn(end.mLine))
			//	end.mColumn = GetLineMaxColumn(end.mLine) - 1;

			u.mRemovedStart = start;
			u.mRemovedEnd = end;
			u.mRemoved = GetText(start, end);

			bool modified = false;

			for (int i = start.mLine; i <= end.mLine; i++) {
				auto &line = _lines[i];
				if (aShift) {
					if (!line.empty()) {
						if (line.front().mChar == '\t') {
							line.erase(line.begin());
							modified = true;
						} else {
							for (int j = 0; j < _tabSize && !line.empty() && line.front().mChar == ' '; j++) {
								line.erase(line.begin());
								modified = true;
							}
						}
					}
				} else {
					line.insert(line.begin(), Glyph('\t', TextEditor::PaletteIndex::Background));
					modified = true;
				}
			}

			if (modified) {
				start = Coordinates(start.mLine, GetCharacterColumn(start.mLine, 0));
				Coordinates rangeEnd;
				if (originalEnd.mColumn != 0) {
					end = Coordinates(end.mLine, GetLineMaxColumn(end.mLine));
					rangeEnd = end;
					u.mAdded = GetText(start, end);
				} else {
					end = Coordinates(originalEnd.mLine, 0);
					rangeEnd = Coordinates(end.mLine - 1, GetLineMaxColumn(end.mLine - 1));
					u.mAdded = GetText(start, rangeEnd);
				}

				u.mAddedStart = start;
				u.mAddedEnd = rangeEnd;
				u.mAfter = _state;

				_state.mSelectionStart = start;
				_state.mSelectionEnd = end;
				AddUndo(u);

				_textChanged = true;

				EnsureCursorVisible();
			}

			return;
		} // c == '\t'
		else {
			u.mRemoved = GetSelectedText();
			u.mRemovedStart = _state.mSelectionStart;
			u.mRemovedEnd = _state.mSelectionEnd;
			DeleteSelection();
		}
	} // HasSelection

	auto coord = GetActualCursorCoordinates();
	u.mAddedStart = coord;

	core_assert(!_lines.empty());

	if (aChar == '\n') {
		InsertLine(coord.mLine + 1);
		auto &line = _lines[coord.mLine];
		auto &newLine = _lines[coord.mLine + 1];

		if (_languageDefinition.mAutoIndentation)
			for (size_t it = 0; it < line.size() && isascii(line[it].mChar) && isblank(line[it].mChar); ++it)
				newLine.push_back(line[it]);

		const size_t whitespaceSize = newLine.size();
		auto cindex = GetCharacterIndex(coord);
		newLine.insert(newLine.end(), line.begin() + cindex, line.end());
		line.erase(cindex, line.size());
		SetCursorPosition(Coordinates(coord.mLine + 1, GetCharacterColumn(coord.mLine + 1, (int)whitespaceSize)));
		u.mAdded = (char)aChar;
	} else {
		char buf[5];
		const int charCount = core::utf8::toUtf8(aChar, buf, sizeof(buf));
		if (charCount <= 0) {
			return;
		}
		buf[charCount] = '\0';
		Line &line = _lines[coord.mLine];
		int cindex = GetCharacterIndex(coord);

		if (_overwrite && cindex < (int)line.size()) {
			size_t d = core::utf8::lengthInt((int)line[cindex].mChar);

			u.mRemovedStart = _state.mCursorPosition;
			u.mRemovedEnd = Coordinates(coord.mLine, GetCharacterColumn(coord.mLine, cindex + (int)d));

			while (d-- > 0 && cindex < (int)line.size()) {
				u.mRemoved += line[cindex].mChar;
				line.erase(line.begin() + cindex);
			}
		}

		for (auto p = buf; *p != '\0'; p++, ++cindex) {
			line.insert(line.begin() + cindex, Glyph(*p, PaletteIndex::Default));
		}
		u.mAdded = buf;

		SetCursorPosition(Coordinates(coord.mLine, GetCharacterColumn(coord.mLine, cindex)));
	}

	_textChanged = true;

	u.mAddedEnd = GetActualCursorCoordinates();
	u.mAfter = _state;

	AddUndo(u);

	Colorize(coord.mLine - 1, 3);
	EnsureCursorVisible();
}

void TextEditor::SetReadOnly(bool aValue) {
	_readOnly = aValue;
}

void TextEditor::SetColorizerEnable(bool aValue) {
	_colorizerEnabled = aValue;
}

void TextEditor::SetCursorPosition(const Coordinates &aPosition) {
	if (_state.mCursorPosition != aPosition) {
		_state.mCursorPosition = aPosition;
		_cursorPositionChanged = true;
		EnsureCursorVisible();
	}
}

void TextEditor::SetSelectionStart(const Coordinates &aPosition) {
	_state.mSelectionStart = SanitizeCoordinates(aPosition);
	if (_state.mSelectionStart > _state.mSelectionEnd)
		std::swap(_state.mSelectionStart, _state.mSelectionEnd);
}

void TextEditor::SetSelectionEnd(const Coordinates &aPosition) {
	_state.mSelectionEnd = SanitizeCoordinates(aPosition);
	if (_state.mSelectionStart > _state.mSelectionEnd)
		std::swap(_state.mSelectionStart, _state.mSelectionEnd);
}

void TextEditor::SetSelection(const Coordinates &aStart, const Coordinates &aEnd, SelectionMode aMode) {
	auto oldSelStart = _state.mSelectionStart;
	auto oldSelEnd = _state.mSelectionEnd;

	_state.mSelectionStart = SanitizeCoordinates(aStart);
	_state.mSelectionEnd = SanitizeCoordinates(aEnd);
	if (_state.mSelectionStart > _state.mSelectionEnd)
		std::swap(_state.mSelectionStart, _state.mSelectionEnd);

	switch (aMode) {
	case TextEditor::SelectionMode::Normal:
		break;
	case TextEditor::SelectionMode::Word: {
		_state.mSelectionStart = FindWordStart(_state.mSelectionStart);
		if (!IsOnWordBoundary(_state.mSelectionEnd))
			_state.mSelectionEnd = FindWordEnd(FindWordStart(_state.mSelectionEnd));
		break;
	}
	case TextEditor::SelectionMode::Line: {
		const auto lineNo = _state.mSelectionEnd.mLine;
		_state.mSelectionStart = Coordinates(_state.mSelectionStart.mLine, 0);
		_state.mSelectionEnd = Coordinates(lineNo, GetLineMaxColumn(lineNo));
		break;
	}
	default:
		break;
	}

	if (_state.mSelectionStart != oldSelStart || _state.mSelectionEnd != oldSelEnd)
		_cursorPositionChanged = true;
}

void TextEditor::SetTabSize(int aValue) {
	_tabSize = core_max(0, core_min(32, aValue));
}

void TextEditor::InsertText(const core::String &aValue) {
	InsertText(aValue.c_str());
}

void TextEditor::InsertText(const char *aValue) {
	if (aValue == nullptr)
		return;

	auto pos = GetActualCursorCoordinates();
	auto start = core_min(pos, _state.mSelectionStart);
	int totalLines = pos.mLine - start.mLine;

	totalLines += InsertTextAt(pos, aValue);

	SetSelection(pos, pos);
	SetCursorPosition(pos);
	Colorize(start.mLine - 1, totalLines + 2);
}

void TextEditor::DeleteSelection() {
	core_assert(_state.mSelectionEnd >= _state.mSelectionStart);

	if (_state.mSelectionEnd == _state.mSelectionStart)
		return;

	DeleteRange(_state.mSelectionStart, _state.mSelectionEnd);

	SetSelection(_state.mSelectionStart, _state.mSelectionStart);
	SetCursorPosition(_state.mSelectionStart);
	Colorize(_state.mSelectionStart.mLine, 1);
}

void TextEditor::MoveUp(int aAmount, bool aSelect) {
	auto oldPos = _state.mCursorPosition;
	_state.mCursorPosition.mLine = core_max(0, _state.mCursorPosition.mLine - aAmount);
	if (oldPos != _state.mCursorPosition) {
		if (aSelect) {
			if (oldPos == _interactiveStart)
				_interactiveStart = _state.mCursorPosition;
			else if (oldPos == _interactiveEnd)
				_interactiveEnd = _state.mCursorPosition;
			else {
				_interactiveStart = _state.mCursorPosition;
				_interactiveEnd = oldPos;
			}
		} else
			_interactiveStart = _interactiveEnd = _state.mCursorPosition;
		SetSelection(_interactiveStart, _interactiveEnd);

		EnsureCursorVisible();
	}
}

void TextEditor::MoveDown(int aAmount, bool aSelect) {
	core_assert(_state.mCursorPosition.mColumn >= 0);
	auto oldPos = _state.mCursorPosition;
	_state.mCursorPosition.mLine =
		core_max(0, core_min((int)_lines.size() - 1, _state.mCursorPosition.mLine + aAmount));

	if (_state.mCursorPosition != oldPos) {
		if (aSelect) {
			if (oldPos == _interactiveEnd)
				_interactiveEnd = _state.mCursorPosition;
			else if (oldPos == _interactiveStart)
				_interactiveStart = _state.mCursorPosition;
			else {
				_interactiveStart = oldPos;
				_interactiveEnd = _state.mCursorPosition;
			}
		} else
			_interactiveStart = _interactiveEnd = _state.mCursorPosition;
		SetSelection(_interactiveStart, _interactiveEnd);

		EnsureCursorVisible();
	}
}

void TextEditor::MoveLeft(int aAmount, bool aSelect, bool aWordMode) {
	if (_lines.empty())
		return;

	auto oldPos = _state.mCursorPosition;
	_state.mCursorPosition = GetActualCursorCoordinates();
	auto line = _state.mCursorPosition.mLine;
	auto cindex = GetCharacterIndex(_state.mCursorPosition);

	while (aAmount-- > 0) {
		if (cindex == 0) {
			if (line > 0) {
				--line;
				if ((int)_lines.size() > line)
					cindex = (int)_lines[line].size();
				else
					cindex = 0;
			}
		} else {
			--cindex;
			if (cindex > 0) {
				if ((int)_lines.size() > line) {
					while (cindex > 0 && core::utf8::isMultibyte(_lines[line][cindex].mChar))
						--cindex;
				}
			}
		}

		_state.mCursorPosition = Coordinates(line, GetCharacterColumn(line, cindex));
		if (aWordMode) {
			_state.mCursorPosition = FindWordStart(_state.mCursorPosition);
			cindex = GetCharacterIndex(_state.mCursorPosition);
		}
	}

	_state.mCursorPosition = Coordinates(line, GetCharacterColumn(line, cindex));

	core_assert(_state.mCursorPosition.mColumn >= 0);
	if (aSelect) {
		if (oldPos == _interactiveStart)
			_interactiveStart = _state.mCursorPosition;
		else if (oldPos == _interactiveEnd)
			_interactiveEnd = _state.mCursorPosition;
		else {
			_interactiveStart = _state.mCursorPosition;
			_interactiveEnd = oldPos;
		}
	} else
		_interactiveStart = _interactiveEnd = _state.mCursorPosition;
	SetSelection(_interactiveStart, _interactiveEnd,
				 aSelect && aWordMode ? SelectionMode::Word : SelectionMode::Normal);

	EnsureCursorVisible();
}

void TextEditor::MoveRight(int aAmount, bool aSelect, bool aWordMode) {
	auto oldPos = _state.mCursorPosition;

	if (_lines.empty() || oldPos.mLine >= (int)_lines.size())
		return;

	auto cindex = GetCharacterIndex(_state.mCursorPosition);
	while (aAmount-- > 0) {
		auto lindex = _state.mCursorPosition.mLine;
		auto &line = _lines[lindex];

		if (cindex >= (int)line.size()) {
			if (_state.mCursorPosition.mLine < (int)_lines.size() - 1) {
				_state.mCursorPosition.mLine =
					core_max(0, core_min((int)_lines.size() - 1, _state.mCursorPosition.mLine + 1));
				_state.mCursorPosition.mColumn = 0;
			} else
				return;
		} else {
			cindex += core::utf8::lengthInt((int)line[cindex].mChar);
			_state.mCursorPosition = Coordinates(lindex, GetCharacterColumn(lindex, cindex));
			if (aWordMode)
				_state.mCursorPosition = FindNextWord(_state.mCursorPosition);
		}
	}

	if (aSelect) {
		if (oldPos == _interactiveEnd)
			_interactiveEnd = SanitizeCoordinates(_state.mCursorPosition);
		else if (oldPos == _interactiveStart)
			_interactiveStart = _state.mCursorPosition;
		else {
			_interactiveStart = oldPos;
			_interactiveEnd = _state.mCursorPosition;
		}
	} else
		_interactiveStart = _interactiveEnd = _state.mCursorPosition;
	SetSelection(_interactiveStart, _interactiveEnd,
				 aSelect && aWordMode ? SelectionMode::Word : SelectionMode::Normal);

	EnsureCursorVisible();
}

void TextEditor::MoveTop(bool aSelect) {
	auto oldPos = _state.mCursorPosition;
	SetCursorPosition(Coordinates(0, 0));

	if (_state.mCursorPosition != oldPos) {
		if (aSelect) {
			_interactiveEnd = oldPos;
			_interactiveStart = _state.mCursorPosition;
		} else
			_interactiveStart = _interactiveEnd = _state.mCursorPosition;
		SetSelection(_interactiveStart, _interactiveEnd);
	}
}

void TextEditor::TextEditor::MoveBottom(bool aSelect) {
	auto oldPos = GetCursorPosition();
	auto newPos = Coordinates((int)_lines.size() - 1, 0);
	SetCursorPosition(newPos);
	if (aSelect) {
		_interactiveStart = oldPos;
		_interactiveEnd = newPos;
	} else
		_interactiveStart = _interactiveEnd = newPos;
	SetSelection(_interactiveStart, _interactiveEnd);
}

void TextEditor::MoveHome(bool aSelect) {
	auto oldPos = _state.mCursorPosition;
	SetCursorPosition(Coordinates(_state.mCursorPosition.mLine, 0));

	if (_state.mCursorPosition != oldPos) {
		if (aSelect) {
			if (oldPos == _interactiveStart)
				_interactiveStart = _state.mCursorPosition;
			else if (oldPos == _interactiveEnd)
				_interactiveEnd = _state.mCursorPosition;
			else {
				_interactiveStart = _state.mCursorPosition;
				_interactiveEnd = oldPos;
			}
		} else
			_interactiveStart = _interactiveEnd = _state.mCursorPosition;
		SetSelection(_interactiveStart, _interactiveEnd);
	}
}

void TextEditor::MoveEnd(bool aSelect) {
	auto oldPos = _state.mCursorPosition;
	SetCursorPosition(Coordinates(_state.mCursorPosition.mLine, GetLineMaxColumn(oldPos.mLine)));

	if (_state.mCursorPosition != oldPos) {
		if (aSelect) {
			if (oldPos == _interactiveEnd)
				_interactiveEnd = _state.mCursorPosition;
			else if (oldPos == _interactiveStart)
				_interactiveStart = _state.mCursorPosition;
			else {
				_interactiveStart = oldPos;
				_interactiveEnd = _state.mCursorPosition;
			}
		} else
			_interactiveStart = _interactiveEnd = _state.mCursorPosition;
		SetSelection(_interactiveStart, _interactiveEnd);
	}
}

void TextEditor::Delete() {
	core_assert(!_readOnly);

	if (_lines.empty())
		return;

	UndoRecord u;
	u.mBefore = _state;

	if (HasSelection()) {
		u.mRemoved = GetSelectedText();
		u.mRemovedStart = _state.mSelectionStart;
		u.mRemovedEnd = _state.mSelectionEnd;

		DeleteSelection();
	} else {
		auto pos = GetActualCursorCoordinates();
		SetCursorPosition(pos);
		auto &line = _lines[pos.mLine];

		if (pos.mColumn == GetLineMaxColumn(pos.mLine)) {
			if (pos.mLine == (int)_lines.size() - 1)
				return;

			u.mRemoved = '\n';
			u.mRemovedStart = u.mRemovedEnd = GetActualCursorCoordinates();
			Advance(u.mRemovedEnd);

			auto &nextLine = _lines[pos.mLine + 1];
			line.insert(line.end(), nextLine.begin(), nextLine.end());
			RemoveLine(pos.mLine + 1);
		} else {
			auto cindex = GetCharacterIndex(pos);
			u.mRemovedStart = u.mRemovedEnd = GetActualCursorCoordinates();
			u.mRemovedEnd.mColumn++;
			u.mRemoved = GetText(u.mRemovedStart, u.mRemovedEnd);

			auto d = core::utf8::lengthInt((int)line[cindex].mChar);
			while (d-- > 0 && cindex < (int)line.size())
				line.erase(line.begin() + cindex);
		}

		_textChanged = true;

		Colorize(pos.mLine, 1);
	}

	u.mAfter = _state;
	AddUndo(u);
}

void TextEditor::Backspace() {
	core_assert(!_readOnly);

	if (_lines.empty())
		return;

	UndoRecord u;
	u.mBefore = _state;

	if (HasSelection()) {
		u.mRemoved = GetSelectedText();
		u.mRemovedStart = _state.mSelectionStart;
		u.mRemovedEnd = _state.mSelectionEnd;

		DeleteSelection();
	} else {
		auto pos = GetActualCursorCoordinates();
		SetCursorPosition(pos);

		if (_state.mCursorPosition.mColumn == 0) {
			if (_state.mCursorPosition.mLine == 0)
				return;

			u.mRemoved = '\n';
			u.mRemovedStart = u.mRemovedEnd = Coordinates(pos.mLine - 1, GetLineMaxColumn(pos.mLine - 1));
			Advance(u.mRemovedEnd);

			auto &line = _lines[_state.mCursorPosition.mLine];
			auto &prevLine = _lines[_state.mCursorPosition.mLine - 1];
			auto prevSize = GetLineMaxColumn(_state.mCursorPosition.mLine - 1);
			prevLine.insert(prevLine.end(), line.begin(), line.end());

			ErrorMarkers etmp;
			for (const auto &i : _errorMarkers)
				etmp.put(i->first - 1 == _state.mCursorPosition.mLine ? i->first - 1 : i->first, i->second);
			_errorMarkers = core::move(etmp);

			RemoveLine(_state.mCursorPosition.mLine);
			--_state.mCursorPosition.mLine;
			_state.mCursorPosition.mColumn = prevSize;
		} else {
			auto &line = _lines[_state.mCursorPosition.mLine];
			auto cindex = GetCharacterIndex(pos) - 1;
			auto cend = cindex + 1;
			while (cindex > 0 && core::utf8::isMultibyte(line[cindex].mChar))
				--cindex;

			// if (cindex > 0 && core::utf8::lengthInt((int)line[cindex].mChar) > 1)
			//	--cindex;

			u.mRemovedStart = u.mRemovedEnd = GetActualCursorCoordinates();
			--u.mRemovedStart.mColumn;
			--_state.mCursorPosition.mColumn;

			while (cindex < (int)line.size() && cend-- > cindex) {
				u.mRemoved += line[cindex].mChar;
				line.erase(line.begin() + cindex);
			}
		}

		_textChanged = true;

		EnsureCursorVisible();
		Colorize(_state.mCursorPosition.mLine, 1);
	}

	u.mAfter = _state;
	AddUndo(u);
}

void TextEditor::SelectWordUnderCursor() {
	auto c = GetCursorPosition();
	SetSelection(FindWordStart(c), FindWordEnd(c));
}

void TextEditor::SelectAll() {
	SetSelection(Coordinates(0, 0), Coordinates((int)_lines.size(), 0));
}

bool TextEditor::HasSelection() const {
	return _state.mSelectionEnd > _state.mSelectionStart;
}

void TextEditor::Copy() {
	if (HasSelection()) {
		ImGui::SetClipboardText(GetSelectedText().c_str());
	} else {
		if (!_lines.empty()) {
			core::String str;
			auto &line = _lines[GetActualCursorCoordinates().mLine];
			for (auto &g : line)
				str += g.mChar;
			ImGui::SetClipboardText(str.c_str());
		}
	}
}

void TextEditor::Cut() {
	if (IsReadOnly()) {
		Copy();
	} else {
		if (HasSelection()) {
			UndoRecord u;
			u.mBefore = _state;
			u.mRemoved = GetSelectedText();
			u.mRemovedStart = _state.mSelectionStart;
			u.mRemovedEnd = _state.mSelectionEnd;

			Copy();
			DeleteSelection();

			u.mAfter = _state;
			AddUndo(u);
		}
	}
}

void TextEditor::Paste() {
	if (IsReadOnly())
		return;

	auto clipText = ImGui::GetClipboardText();
	if (clipText != nullptr && strlen(clipText) > 0) {
		UndoRecord u;
		u.mBefore = _state;

		if (HasSelection()) {
			u.mRemoved = GetSelectedText();
			u.mRemovedStart = _state.mSelectionStart;
			u.mRemovedEnd = _state.mSelectionEnd;
			DeleteSelection();
		}

		u.mAdded = clipText;
		u.mAddedStart = GetActualCursorCoordinates();

		InsertText(clipText);

		u.mAddedEnd = GetActualCursorCoordinates();
		u.mAfter = _state;
		AddUndo(u);
	}
}

bool TextEditor::CanUndo() const {
	return !_readOnly && _undoIndex > 0;
}

bool TextEditor::CanRedo() const {
	return !_readOnly && _undoIndex < (int)_undoBuffer.size();
}

void TextEditor::Undo(int aSteps) {
	while (CanUndo() && aSteps-- > 0)
		_undoBuffer[--_undoIndex].Undo(this);
}

void TextEditor::Redo(int aSteps) {
	while (CanRedo() && aSteps-- > 0)
		_undoBuffer[_undoIndex++].Redo(this);
}

const TextEditor::Palette &TextEditor::GetDarkPalette() {
	const static Palette p = {{
		0xff7f7f7f, // Default
		0xffd69c56, // Keyword
		0xff00ff00, // Number
		0xff7070e0, // String
		0xff70a0e0, // Char literal
		0xffffffff, // Punctuation
		0xff408080, // Preprocessor
		0xffaaaaaa, // Identifier
		0xff9bc64d, // Known identifier
		0xffc040a0, // Preproc identifier
		0xff206020, // Comment (single line)
		0xff406020, // Comment (multi line)
		0xff101010, // Background
		0xffe0e0e0, // Cursor
		0x80a06020, // Selection
		0x800020ff, // ErrorMarker
		0x40f08000, // Breakpoint
		0xff707000, // Line number
		0x40000000, // Current line fill
		0x40808080, // Current line fill (inactive)
		0x40a0a0a0, // Current line edge
	}};
	return p;
}

const TextEditor::Palette &TextEditor::GetLightPalette() {
	const static Palette p = {{
		0xff7f7f7f, // None
		0xffff0c06, // Keyword
		0xff008000, // Number
		0xff2020a0, // String
		0xff304070, // Char literal
		0xff000000, // Punctuation
		0xff406060, // Preprocessor
		0xff404040, // Identifier
		0xff606010, // Known identifier
		0xffc040a0, // Preproc identifier
		0xff205020, // Comment (single line)
		0xff405020, // Comment (multi line)
		0xffffffff, // Background
		0xff000000, // Cursor
		0x80600000, // Selection
		0xa00010ff, // ErrorMarker
		0x80f08000, // Breakpoint
		0xff505000, // Line number
		0x40000000, // Current line fill
		0x40808080, // Current line fill (inactive)
		0x40000000, // Current line edge
	}};
	return p;
}

const TextEditor::Palette &TextEditor::GetRetroBluePalette() {
	const static Palette p = {{
		0xff00ffff, // None
		0xffffff00, // Keyword
		0xff00ff00, // Number
		0xff808000, // String
		0xff808000, // Char literal
		0xffffffff, // Punctuation
		0xff008000, // Preprocessor
		0xff00ffff, // Identifier
		0xffffffff, // Known identifier
		0xffff00ff, // Preproc identifier
		0xff808080, // Comment (single line)
		0xff404040, // Comment (multi line)
		0xff800000, // Background
		0xff0080ff, // Cursor
		0x80ffff00, // Selection
		0xa00000ff, // ErrorMarker
		0x80ff8000, // Breakpoint
		0xff808000, // Line number
		0x40000000, // Current line fill
		0x40808080, // Current line fill (inactive)
		0x40000000, // Current line edge
	}};
	return p;
}

core::String TextEditor::GetText() const {
	return GetText(Coordinates(), Coordinates((int)_lines.size(), 0));
}

core::DynamicArray<core::String> TextEditor::GetTextLines() const {
	core::DynamicArray<core::String> result;

	result.reserve(_lines.size());

	for (auto &line : _lines) {
		core::String text;

		text.reserve(line.size());

		for (size_t i = 0; i < line.size(); ++i)
			text += line[i].mChar;

		result.emplace_back(std::move(text));
	}

	return result;
}

core::String TextEditor::GetSelectedText() const {
	return GetText(_state.mSelectionStart, _state.mSelectionEnd);
}

core::String TextEditor::GetCurrentLineText() const {
	auto lineLength = GetLineMaxColumn(_state.mCursorPosition.mLine);
	return GetText(Coordinates(_state.mCursorPosition.mLine, 0), Coordinates(_state.mCursorPosition.mLine, lineLength));
}

void TextEditor::ProcessInputs() {
}

void TextEditor::Colorize(int aFromLine, int aLines) {
	int toLine = aLines == -1 ? (int)_lines.size() : core_min((int)_lines.size(), aFromLine + aLines);
	_colorRangeMin = core_min(_colorRangeMin, aFromLine);
	_colorRangeMax = core_max(_colorRangeMax, toLine);
	_colorRangeMin = core_max(0, _colorRangeMin);
	_colorRangeMax = core_max(_colorRangeMin, _colorRangeMax);
	_checkComments = true;
}

void TextEditor::ColorizeRange(int aFromLine, int aToLine) {
	if (_lines.empty() || aFromLine >= aToLine) {
		return;
	}

	core::String buffer;
	std::cmatch results;
	core::String id;

	int endLine = core_max(0, core_min((int)_lines.size(), aToLine));
	for (int i = aFromLine; i < endLine; ++i) {
		Line &line = _lines[i];

		if (line.empty()) {
			continue;
		}

		buffer.clear();
		buffer.reserve(line.size());
		for (size_t j = 0; j < line.size(); ++j) {
			Glyph &col = line[j];
			buffer += col.mChar;
			col.mColorIndex = PaletteIndex::Default;
		}

		const char *bufferBegin = &buffer[0];
		const char *bufferEnd = bufferBegin + buffer.size();

		const char *last = bufferEnd;

		for (auto first = bufferBegin; first != last;) {
			const char *token_begin = nullptr;
			const char *token_end = nullptr;
			PaletteIndex token_color = PaletteIndex::Default;

			bool hasTokenizeResult = false;

			if (_languageDefinition.mTokenize != nullptr) {
				if (_languageDefinition.mTokenize(first, last, token_begin, token_end, token_color)) {
					hasTokenizeResult = true;
				}
			}

			if (hasTokenizeResult == false) {
				// todo : remove
				// printf("using regex for %.*s\n", first + 10 < last ? 10 : int(last - first), first);

				for (auto &p : _regexList) {
					if (std::regex_search(first, last, results, p.first, std::regex_constants::match_continuous)) {
						hasTokenizeResult = true;

						auto &v = *results.begin();
						token_begin = v.first;
						token_end = v.second;
						token_color = p.second;
						break;
					}
				}
			}

			if (!hasTokenizeResult) {
				++first;
				continue;
			}
			const size_t token_length = token_end - token_begin;

			if (token_color == PaletteIndex::Identifier) {
				id = core::String(token_begin, (size_t)(token_end - token_begin));

				// todo : allmost all language definitions use lower case to specify keywords, so shouldn't this use
				// ::tolower ?
				if (!_languageDefinition.mCaseSensitive) {
					id = id.toUpper();
				}

				const uint32_t lineIdx = first - bufferBegin;
				if (lineIdx < line.size() && !line[lineIdx].mPreprocessor) {
					if (_languageDefinition.mKeywords.has(id)) {
						token_color = PaletteIndex::Keyword;
					} else if (_languageDefinition.mIdentifiers.hasKey(id)) {
						token_color = PaletteIndex::KnownIdentifier;
					} else if (_languageDefinition.mPreprocIdentifiers.hasKey(id)) {
						token_color = PaletteIndex::PreprocIdentifier;
					}
				} else {
					if (_languageDefinition.mPreprocIdentifiers.hasKey(id)) {
						token_color = PaletteIndex::PreprocIdentifier;
					}
				}
			}

			for (size_t j = 0; j < token_length; ++j) {
				const uint32_t lineIdx = (token_begin - bufferBegin) + j;
				if (lineIdx < line.size()) {
					line[lineIdx].mColorIndex = token_color;
				}
			}

			first = token_end;
		}
	}
}

void TextEditor::ColorizeInternal() {
	if (_lines.empty() || !_colorizerEnabled)
		return;

	if (_checkComments) {
		size_t endLine = _lines.size();
		int endIndex = 0;
		size_t commentStartLine = endLine;
		int commentStartIndex = endIndex;
		bool withinString = false;
		bool withinSingleLineComment = false;
		bool withinPreproc = false;
		bool firstChar = true;	  // there is no other non-whitespace characters in the line before
		bool concatenate = false; // '\' on the very end of the line
		size_t currentLine = 0;
		int currentIndex = 0;
		while (currentLine < endLine || currentIndex < endIndex) {
			TextEditor::Line &line = _lines[currentLine];

			if (currentIndex == 0 && !concatenate) {
				withinSingleLineComment = false;
				withinPreproc = false;
				firstChar = true;
			}

			concatenate = false;

			if (!line.empty()) {
				TextEditor::Glyph &g = line[currentIndex];
				TextEditor::Char c = g.mChar;

				if (c != (Char)_languageDefinition.mPreprocChar && !SDL_isspace(c)) {
					firstChar = false;
				}

				if (currentIndex == (int)line.size() - 1 && line[line.size() - 1].mChar == '\\')
					concatenate = true;

				bool inComment = (commentStartLine < currentLine ||
								  (commentStartLine == currentLine && commentStartIndex <= currentIndex));

				if (withinString) {
					line[currentIndex].mMultiLineComment = inComment;

					if (c == '\"') {
						if (currentIndex + 1 < (int)line.size() && line[currentIndex + 1].mChar == '\"') {
							currentIndex += 1;
							if (currentIndex < (int)line.size())
								line[currentIndex].mMultiLineComment = inComment;
						} else
							withinString = false;
					} else if (c == '\\') {
						currentIndex += 1;
						if (currentIndex < (int)line.size())
							line[currentIndex].mMultiLineComment = inComment;
					}
				} else {
					if (firstChar && c == (Char)_languageDefinition.mPreprocChar)
						withinPreproc = true;

					if (c == '\"') {
						withinString = true;
						line[currentIndex].mMultiLineComment = inComment;
					} else {
						auto pred = [](const char &a, const Glyph &b) { return (Char)a == b.mChar; };
						auto from = line.begin() + currentIndex;
						core::String &startStr = _languageDefinition.mCommentStart;
						core::String &singleStartStr = _languageDefinition.mSingleLineComment;

						if (singleStartStr.size() > 0 && currentIndex + singleStartStr.size() <= line.size() &&
							equals(singleStartStr.begin(), singleStartStr.end(), from, from + singleStartStr.size(),
								   pred)) {
							withinSingleLineComment = true;
						} else if (!withinSingleLineComment && currentIndex + startStr.size() <= line.size() &&
								   equals(startStr.begin(), startStr.end(), from, from + startStr.size(), pred)) {
							commentStartLine = currentLine;
							commentStartIndex = currentIndex;
						}

						inComment = (commentStartLine < currentLine ||
									 (commentStartLine == currentLine && commentStartIndex <= currentIndex));

						line[currentIndex].mMultiLineComment = inComment;
						line[currentIndex].mComment = withinSingleLineComment;

						core::String &endStr = _languageDefinition.mCommentEnd;
						if (currentIndex + 1 >= (int)endStr.size() &&
							equals(endStr.begin(), endStr.end(), from + 1 - endStr.size(), from + 1, pred)) {
							commentStartIndex = endIndex;
							commentStartLine = endLine;
						}
					}
				}
				line[currentIndex].mPreprocessor = withinPreproc;
				currentIndex += core::utf8::lengthInt((int)c);
				if (currentIndex >= (int)line.size()) {
					currentIndex = 0;
					++currentLine;
				}
			} else {
				currentIndex = 0;
				++currentLine;
			}
		}
		_checkComments = false;
	}

	if (_colorRangeMin < _colorRangeMax) {
		const int increment = (_languageDefinition.mTokenize == nullptr) ? 10 : 10000;
		const int to = core_min(_colorRangeMin + increment, _colorRangeMax);
		ColorizeRange(_colorRangeMin, to);
		_colorRangeMin = to;

		if (_colorRangeMax == _colorRangeMin) {
			_colorRangeMin = std::numeric_limits<int>::max();
			_colorRangeMax = 0;
		}
		return;
	}
}

float TextEditor::TextDistanceToLineStart(const Coordinates &aFrom) const {
	auto &line = _lines[aFrom.mLine];
	float distance = 0.0f;
	float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;
	int colIndex = GetCharacterIndex(aFrom);
	for (size_t it = 0u; it < line.size() && it < (size_t)colIndex;) {
		if (line[it].mChar == '\t') {
			distance =
				(1.0f + glm::floor((1.0f + distance) / (float(_tabSize) * spaceSize))) * (float(_tabSize) * spaceSize);
			++it;
		} else {
			auto d = core::utf8::lengthInt((int)line[it].mChar);
			char tempCString[7];
			int i = 0;
			for (; i < 6 && d-- > 0 && it < line.size(); i++, it++)
				tempCString[i] = line[it].mChar;

			tempCString[i] = '\0';
			distance +=
				ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, tempCString, nullptr, nullptr).x;
		}
	}

	return distance;
}

void TextEditor::EnsureCursorVisible() {
	if (!_withinRender) {
		_scrollToCursor = true;
		return;
	}

	float scrollX = ImGui::GetScrollX();
	float scrollY = ImGui::GetScrollY();

	float height = ImGui::GetWindowHeight();
	float width = ImGui::GetWindowWidth();

	int top = 1 + (int)glm::ceil(scrollY / _charAdvance.y);
	int bottom = (int)glm::ceil((scrollY + height) / _charAdvance.y);

	int left = (int)glm::ceil(scrollX / _charAdvance.x);
	int right = (int)glm::ceil((scrollX + width) / _charAdvance.x);

	auto pos = GetActualCursorCoordinates();
	float len = TextDistanceToLineStart(pos);

	if (pos.mLine < top)
		ImGui::SetScrollY(core_max(0.0f, (pos.mLine - 1) * _charAdvance.y));
	if (pos.mLine > bottom - 4)
		ImGui::SetScrollY(core_max(0.0f, (pos.mLine + 4) * _charAdvance.y - height));
	if (len + _textStart < (float)left + 4.0f)
		ImGui::SetScrollX(core_max(0.0f, len + _textStart - 4));
	if (len + _textStart > (float)right - 4.0f)
		ImGui::SetScrollX(core_max(0.0f, len + _textStart + 4 - width));
}

int TextEditor::GetPageSize() const {
	float height = ImGui::GetWindowHeight() - 20.0f;
	return (int)glm::floor(height / _charAdvance.y);
}

TextEditor::UndoRecord::UndoRecord(const core::String &aAdded, const TextEditor::Coordinates aAddedStart,
								   const TextEditor::Coordinates aAddedEnd, const core::String &aRemoved,
								   const TextEditor::Coordinates aRemovedStart,
								   const TextEditor::Coordinates aRemovedEnd, TextEditor::EditorState &aBefore,
								   TextEditor::EditorState &aAfter)
	: mAdded(aAdded), mAddedStart(aAddedStart), mAddedEnd(aAddedEnd), mRemoved(aRemoved), mRemovedStart(aRemovedStart),
	  mRemovedEnd(aRemovedEnd), mBefore(aBefore), mAfter(aAfter) {
	core_assert(mAddedStart <= mAddedEnd);
	core_assert(mRemovedStart <= mRemovedEnd);
}

void TextEditor::UndoRecord::Undo(TextEditor *aEditor) {
	if (!mAdded.empty()) {
		aEditor->DeleteRange(mAddedStart, mAddedEnd);
		aEditor->Colorize(mAddedStart.mLine - 1, mAddedEnd.mLine - mAddedStart.mLine + 2);
	}

	if (!mRemoved.empty()) {
		auto start = mRemovedStart;
		aEditor->InsertTextAt(start, mRemoved.c_str());
		aEditor->Colorize(mRemovedStart.mLine - 1, mRemovedEnd.mLine - mRemovedStart.mLine + 2);
	}

	aEditor->_state = mBefore;
	aEditor->EnsureCursorVisible();
}

void TextEditor::UndoRecord::Redo(TextEditor *aEditor) {
	if (!mRemoved.empty()) {
		aEditor->DeleteRange(mRemovedStart, mRemovedEnd);
		aEditor->Colorize(mRemovedStart.mLine - 1, mRemovedEnd.mLine - mRemovedStart.mLine + 1);
	}

	if (!mAdded.empty()) {
		auto start = mAddedStart;
		aEditor->InsertTextAt(start, mAdded.c_str());
		aEditor->Colorize(mAddedStart.mLine - 1, mAddedEnd.mLine - mAddedStart.mLine + 1);
	}

	aEditor->_state = mAfter;
	aEditor->EnsureCursorVisible();
}

static bool TokenizeCStyleString(const char *in_begin, const char *in_end, const char *&out_begin,
								 const char *&out_end) {
	const char *p = in_begin;

	if (*p == '"') {
		p++;

		while (p < in_end) {
			// handle end of string
			if (*p == '"') {
				out_begin = in_begin;
				out_end = p + 1;
				return true;
			}

			// handle escape character for "
			if (*p == '\\' && p + 1 < in_end && p[1] == '"')
				p++;

			p++;
		}
	}

	return false;
}

static bool TokenizeCStyleCharacterLiteral(const char *in_begin, const char *in_end, const char *&out_begin,
										   const char *&out_end) {
	const char *p = in_begin;

	if (*p == '\'') {
		p++;

		// handle escape characters
		if (p < in_end && *p == '\\')
			p++;

		if (p < in_end)
			p++;

		// handle end of character literal
		if (p < in_end && *p == '\'') {
			out_begin = in_begin;
			out_end = p + 1;
			return true;
		}
	}

	return false;
}

static bool TokenizeCStyleIdentifier(const char *in_begin, const char *in_end, const char *&out_begin,
									 const char *&out_end) {
	const char *p = in_begin;

	if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_') {
		p++;

		while ((p < in_end) &&
			   ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_'))
			p++;

		out_begin = in_begin;
		out_end = p;
		return true;
	}

	return false;
}

static bool TokenizeCStyleNumber(const char *in_begin, const char *in_end, const char *&out_begin,
								 const char *&out_end) {
	const char *p = in_begin;

	const bool startsWithNumber = *p >= '0' && *p <= '9';

	if (*p != '+' && *p != '-' && !startsWithNumber)
		return false;

	p++;

	bool hasNumber = startsWithNumber;

	while (p < in_end && (*p >= '0' && *p <= '9')) {
		hasNumber = true;

		p++;
	}

	if (hasNumber == false)
		return false;

	bool isFloat = false;
	bool isHex = false;
	bool isBinary = false;

	if (p < in_end) {
		if (*p == '.') {
			isFloat = true;

			p++;

			while (p < in_end && (*p >= '0' && *p <= '9'))
				p++;
		} else if (*p == 'x' || *p == 'X') {
			// hex formatted integer of the type 0xef80

			isHex = true;

			p++;

			while (p < in_end && ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')))
				p++;
		} else if (*p == 'b' || *p == 'B') {
			// binary formatted integer of the type 0b01011101

			isBinary = true;

			p++;

			while (p < in_end && (*p >= '0' && *p <= '1'))
				p++;
		}
	}

	if (isHex == false && isBinary == false) {
		// floating point exponent
		if (p < in_end && (*p == 'e' || *p == 'E')) {
			isFloat = true;

			p++;

			if (p < in_end && (*p == '+' || *p == '-'))
				p++;

			bool hasDigits = false;

			while (p < in_end && (*p >= '0' && *p <= '9')) {
				hasDigits = true;

				p++;
			}

			if (hasDigits == false)
				return false;
		}

		// single precision floating point type
		if (p < in_end && *p == 'f')
			p++;
	}

	if (isFloat == false) {
		// integer size type
		while (p < in_end && (*p == 'u' || *p == 'U' || *p == 'l' || *p == 'L'))
			p++;
	}

	out_begin = in_begin;
	out_end = p;
	return true;
}

static bool TokenizeCStylePunctuation(const char *in_begin, const char *in_end, const char *&out_begin,
									  const char *&out_end) {
	(void)in_end;

	switch (*in_begin) {
	case '[':
	case ']':
	case '{':
	case '}':
	case '!':
	case '%':
	case '^':
	case '&':
	case '*':
	case '(':
	case ')':
	case '-':
	case '+':
	case '=':
	case '~':
	case '|':
	case '<':
	case '>':
	case '?':
	case ':':
	case '/':
	case ';':
	case ',':
	case '.':
		out_begin = in_begin;
		out_end = in_begin + 1;
		return true;
	}

	return false;
}

const TextEditor::LanguageDefinition &TextEditor::LanguageDefinition::CPlusPlus() {
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited) {
		static const char *const cppKeywords[] = {"alignas",
												  "alignof",
												  "and",
												  "and_eq",
												  "asm",
												  "atomic_cancel",
												  "atomic_commit",
												  "atomic_noexcept",
												  "auto",
												  "bitand",
												  "bitor",
												  "bool",
												  "break",
												  "case",
												  "catch",
												  "char",
												  "char16_t",
												  "char32_t",
												  "class",
												  "compl",
												  "concept",
												  "const",
												  "constexpr",
												  "const_cast",
												  "continue",
												  "decltype",
												  "default",
												  "delete",
												  "do",
												  "double",
												  "dynamic_cast",
												  "else",
												  "enum",
												  "explicit",
												  "export",
												  "extern",
												  "false",
												  "float",
												  "for",
												  "friend",
												  "goto",
												  "if",
												  "import",
												  "inline",
												  "int",
												  "long",
												  "module",
												  "mutable",
												  "namespace",
												  "new",
												  "noexcept",
												  "not",
												  "not_eq",
												  "nullptr",
												  "operator",
												  "or",
												  "or_eq",
												  "private",
												  "protected",
												  "public",
												  "register",
												  "reinterpret_cast",
												  "requires",
												  "return",
												  "short",
												  "signed",
												  "sizeof",
												  "static",
												  "static_assert",
												  "static_cast",
												  "struct",
												  "switch",
												  "synchronized",
												  "template",
												  "this",
												  "thread_local",
												  "throw",
												  "true",
												  "try",
												  "typedef",
												  "typeid",
												  "typename",
												  "union",
												  "unsigned",
												  "using",
												  "virtual",
												  "void",
												  "volatile",
												  "wchar_t",
												  "while",
												  "xor",
												  "xor_eq"};
		for (auto &k : cppKeywords)
			langDef.mKeywords.insert(k);

		static const char *const identifiers[] = {
			"abort",		 "abs",		"acos",			 "asin",	"atan",		"atexit",  "atof",	  "atoi",
			"atol",			 "ceil",	"clock",		 "cosh",	"ctime",	"div",	   "exit",	  "fabs",
			"floor",		 "fmod",	"getchar",		 "getenv",	"isalnum",	"isalpha", "isdigit", "isgraph",
			"ispunct",		 "isspace", "isupper",		 "kbhit",	"log10",	"log2",	   "log",	  "memcmp",
			"modf",			 "pow",		"printf",		 "sprintf", "snprintf", "putchar", "putenv",  "puts",
			"rand",			 "remove",	"rename",		 "sinh",	"sqrt",		"srand",   "strcat",  "strcmp",
			"strerror",		 "time",	"tolower",		 "toupper", "std",		"string",  "vector",  "map",
			"unordered_map", "set",		"unordered_set", "min",		"max"};
		for (auto &k : identifiers) {
			Identifier id;
			id.mDeclaration = "Built-in function";
			langDef.mIdentifiers.put(core::String(k), id);
		}

		langDef.mTokenize = [](const char *in_begin, const char *in_end, const char *&out_begin, const char *&out_end,
							   PaletteIndex &paletteIndex) -> bool {
			paletteIndex = PaletteIndex::Max;

			while (in_begin < in_end && isascii(*in_begin) && isblank(*in_begin))
				in_begin++;

			if (in_begin == in_end) {
				out_begin = in_end;
				out_end = in_end;
				paletteIndex = PaletteIndex::Default;
			} else if (TokenizeCStyleString(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::String;
			else if (TokenizeCStyleCharacterLiteral(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::CharLiteral;
			else if (TokenizeCStyleIdentifier(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::Identifier;
			else if (TokenizeCStyleNumber(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::Number;
			else if (TokenizeCStylePunctuation(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::Punctuation;

			return paletteIndex != PaletteIndex::Max;
		};

		langDef.mCommentStart = "/*";
		langDef.mCommentEnd = "*/";
		langDef.mSingleLineComment = "//";

		langDef.mCaseSensitive = true;
		langDef.mAutoIndentation = true;

		langDef.mName = "C++";

		inited = true;
	}
	return langDef;
}

const TextEditor::LanguageDefinition &TextEditor::LanguageDefinition::GLSL() {
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited) {
		static const char *const keywords[] = {
			"auto",		  "break",	   "case",			 "char",		 "const",	 "continue", "default",	 "do",
			"double",	  "else",	   "enum",			 "extern",		 "float",	 "for",		 "goto",	 "if",
			"inline",	  "int",	   "long",			 "register",	 "restrict", "return",	 "short",	 "signed",
			"sizeof",	  "static",	   "struct",		 "switch",		 "typedef",	 "union",	 "unsigned", "void",
			"volatile",	  "while",	   "_Alignas",		 "_Alignof",	 "_Atomic",	 "_Bool",	 "_Complex", "_Generic",
			"_Imaginary", "_Noreturn", "_Static_assert", "_Thread_local"};
		for (auto &k : keywords)
			langDef.mKeywords.insert(k);

		static const char *const identifiers[] = {
			"abort",	"abs",	  "acos",	 "asin",	"atan",	   "atexit",  "atof",	 "atoi",	"atol",
			"ceil",		"clock",  "cosh",	 "ctime",	"div",	   "exit",	  "fabs",	 "floor",	"fmod",
			"getchar",	"getenv", "isalnum", "isalpha", "isdigit", "isgraph", "ispunct", "isspace", "isupper",
			"kbhit",	"log10",  "log2",	 "log",		"memcmp",  "modf",	  "pow",	 "putchar", "putenv",
			"puts",		"rand",	  "remove",	 "rename",	"sinh",	   "sqrt",	  "srand",	 "strcat",	"strcmp",
			"strerror", "time",	  "tolower", "toupper"};
		for (auto &k : identifiers) {
			Identifier id;
			id.mDeclaration = "Built-in function";
			langDef.mIdentifiers.put(core::String(k), id);
		}

		langDef.mTokenRegexStrings.push_back(
			std::make_pair<core::String, PaletteIndex>("[ \\t]*#[ \\t]*[a-zA-Z_]+", PaletteIndex::Preprocessor));
		langDef.mTokenRegexStrings.push_back(
			std::make_pair<core::String, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
		langDef.mTokenRegexStrings.push_back(
			std::make_pair<core::String, PaletteIndex>("\\'\\\\?[^\\']\\'", PaletteIndex::CharLiteral));
		langDef.mTokenRegexStrings.push_back(std::make_pair<core::String, PaletteIndex>(
			"[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(
			std::make_pair<core::String, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(
			std::make_pair<core::String, PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(
			std::make_pair<core::String, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(
			std::make_pair<core::String, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.mTokenRegexStrings.push_back(std::make_pair<core::String, PaletteIndex>(
			"[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

		langDef.mCommentStart = "/*";
		langDef.mCommentEnd = "*/";
		langDef.mSingleLineComment = "//";

		langDef.mCaseSensitive = true;
		langDef.mAutoIndentation = true;

		langDef.mName = "GLSL";

		inited = true;
	}
	return langDef;
}

const TextEditor::LanguageDefinition &TextEditor::LanguageDefinition::C() {
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited) {
		static const char *const keywords[] = {
			"auto",		  "break",	   "case",			 "char",		 "const",	 "continue", "default",	 "do",
			"double",	  "else",	   "enum",			 "extern",		 "float",	 "for",		 "goto",	 "if",
			"inline",	  "int",	   "long",			 "register",	 "restrict", "return",	 "short",	 "signed",
			"sizeof",	  "static",	   "struct",		 "switch",		 "typedef",	 "union",	 "unsigned", "void",
			"volatile",	  "while",	   "_Alignas",		 "_Alignof",	 "_Atomic",	 "_Bool",	 "_Complex", "_Generic",
			"_Imaginary", "_Noreturn", "_Static_assert", "_Thread_local"};
		for (auto &k : keywords)
			langDef.mKeywords.insert(k);

		static const char *const identifiers[] = {
			"abort",	"abs",	  "acos",	 "asin",	"atan",	   "atexit",  "atof",	 "atoi",	"atol",
			"ceil",		"clock",  "cosh",	 "ctime",	"div",	   "exit",	  "fabs",	 "floor",	"fmod",
			"getchar",	"getenv", "isalnum", "isalpha", "isdigit", "isgraph", "ispunct", "isspace", "isupper",
			"kbhit",	"log10",  "log2",	 "log",		"memcmp",  "modf",	  "pow",	 "putchar", "putenv",
			"puts",		"rand",	  "remove",	 "rename",	"sinh",	   "sqrt",	  "srand",	 "strcat",	"strcmp",
			"strerror", "time",	  "tolower", "toupper"};
		for (auto &k : identifiers) {
			Identifier id;
			id.mDeclaration = "Built-in function";
			langDef.mIdentifiers.put(core::String(k), id);
		}

		langDef.mTokenize = [](const char *in_begin, const char *in_end, const char *&out_begin, const char *&out_end,
							   PaletteIndex &paletteIndex) -> bool {
			paletteIndex = PaletteIndex::Max;

			while (in_begin < in_end && isascii(*in_begin) && isblank(*in_begin))
				in_begin++;

			if (in_begin == in_end) {
				out_begin = in_end;
				out_end = in_end;
				paletteIndex = PaletteIndex::Default;
			} else if (TokenizeCStyleString(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::String;
			else if (TokenizeCStyleCharacterLiteral(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::CharLiteral;
			else if (TokenizeCStyleIdentifier(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::Identifier;
			else if (TokenizeCStyleNumber(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::Number;
			else if (TokenizeCStylePunctuation(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::Punctuation;

			return paletteIndex != PaletteIndex::Max;
		};

		langDef.mCommentStart = "/*";
		langDef.mCommentEnd = "*/";
		langDef.mSingleLineComment = "//";

		langDef.mCaseSensitive = true;
		langDef.mAutoIndentation = true;

		langDef.mName = "C";

		inited = true;
	}
	return langDef;
}

const TextEditor::LanguageDefinition &TextEditor::LanguageDefinition::Lua() {
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited) {
		static const char *const keywords[] = {"and", "break",	  "do",		"",		"else", "elseif", "end",  "false",
											   "for", "function", "if",		"in",	"",		"local",  "nil",  "not",
											   "or",  "repeat",	  "return", "then", "true", "until",  "while"};

		for (auto &k : keywords)
			langDef.mKeywords.insert(k);

		static const char *const identifiers[] = {"assert",		  "collectgarbage",
												  "dofile",		  "error",
												  "getmetatable", "ipairs",
												  "loadfile",	  "load",
												  "loadstring",	  "next",
												  "pairs",		  "pcall",
												  "print",		  "rawequal",
												  "rawlen",		  "rawget",
												  "rawset",		  "select",
												  "setmetatable", "tonumber",
												  "tostring",	  "type",
												  "xpcall",		  "_G",
												  "_VERSION",	  "arshift",
												  "band",		  "bnot",
												  "bor",		  "bxor",
												  "btest",		  "extract",
												  "lrotate",	  "lshift",
												  "replace",	  "rrotate",
												  "rshift",		  "create",
												  "resume",		  "running",
												  "status",		  "wrap",
												  "yield",		  "isyieldable",
												  "debug",		  "getuservalue",
												  "gethook",	  "getinfo",
												  "getlocal",	  "getregistry",
												  "getmetatable", "getupvalue",
												  "upvaluejoin",  "upvalueid",
												  "setuservalue", "sethook",
												  "setlocal",	  "setmetatable",
												  "setupvalue",	  "traceback",
												  "close",		  "flush",
												  "input",		  "lines",
												  "open",		  "output",
												  "popen",		  "read",
												  "tmpfile",	  "type",
												  "write",		  "close",
												  "flush",		  "lines",
												  "read",		  "seek",
												  "setvbuf",	  "write",
												  "__gc",		  "__tostring",
												  "abs",		  "acos",
												  "asin",		  "atan",
												  "ceil",		  "cos",
												  "deg",		  "exp",
												  "tointeger",	  "floor",
												  "fmod",		  "ult",
												  "log",		  "max",
												  "min",		  "modf",
												  "rad",		  "random",
												  "randomseed",	  "sin",
												  "sqrt",		  "string",
												  "tan",		  "type",
												  "atan2",		  "cosh",
												  "sinh",		  "tanh",
												  "pow",		  "frexp",
												  "ldexp",		  "log10",
												  "pi",			  "huge",
												  "maxinteger",	  "mininteger",
												  "loadlib",	  "searchpath",
												  "seeall",		  "preload",
												  "cpath",		  "path",
												  "searchers",	  "loaded",
												  "module",		  "require",
												  "clock",		  "date",
												  "difftime",	  "execute",
												  "exit",		  "getenv",
												  "remove",		  "rename",
												  "setlocale",	  "time",
												  "tmpname",	  "byte",
												  "char",		  "dump",
												  "find",		  "format",
												  "gmatch",		  "gsub",
												  "len",		  "lower",
												  "match",		  "rep",
												  "reverse",	  "sub",
												  "upper",		  "pack",
												  "packsize",	  "unpack",
												  "concat",		  "maxn",
												  "insert",		  "pack",
												  "unpack",		  "remove",
												  "move",		  "sort",
												  "offset",		  "codepoint",
												  "char",		  "len",
												  "codes",		  "charpattern",
												  "coroutine",	  "table",
												  "io",			  "os",
												  "string",		  "utf8",
												  "bit32",		  "math",
												  "debug",		  "package"};
		for (auto &k : identifiers) {
			Identifier id;
			id.mDeclaration = "Built-in function";
			langDef.mIdentifiers.put(core::String(k), id);
		}

		langDef.mTokenRegexStrings.push_back(
			std::make_pair<core::String, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
		langDef.mTokenRegexStrings.push_back(
			std::make_pair<core::String, PaletteIndex>("\\\'[^\\\']*\\\'", PaletteIndex::String));
		langDef.mTokenRegexStrings.push_back(
			std::make_pair<core::String, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<core::String, PaletteIndex>(
			"[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(
			std::make_pair<core::String, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(
			std::make_pair<core::String, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.mTokenRegexStrings.push_back(std::make_pair<core::String, PaletteIndex>(
			"[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

		langDef.mCommentStart = "--[[";
		langDef.mCommentEnd = "]]";
		langDef.mSingleLineComment = "--";

		langDef.mCaseSensitive = true;
		langDef.mAutoIndentation = false;

		langDef.mName = "Lua";

		inited = true;
	}
	return langDef;
}
