/**
 * ImGuiColorTextEdit (MIT Licensed)
 *
 * https://github.com/BalazsJako/ImGuiColorTextEdit
 */

#pragma once

#include "IMGUI.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Map.h"
#include "core/collection/StringMap.h"
#include "core/collection/StringSet.h"
#include <regex>
#include <utility>

class TextEditor {
public:
	enum class PaletteIndex {
		Default,
		Keyword,
		Number,
		String,
		CharLiteral,
		Punctuation,
		Preprocessor,
		Identifier,
		KnownIdentifier,
		PreprocIdentifier,
		Comment,
		MultiLineComment,
		Background,
		Cursor,
		Selection,
		ErrorMarker,
		Breakpoint,
		LineNumber,
		CurrentLineFill,
		CurrentLineFillInactive,
		CurrentLineEdge,
		Max
	};

	enum class SelectionMode { Normal, Word, Line };

	struct Breakpoint {
		int mLine;
		bool mEnabled;
		core::String mCondition;

		Breakpoint() : mLine(-1), mEnabled(false) {
		}
	};

	// Represents a character coordinate from the user's point of view,
	// i. e. consider an uniform grid (assuming fixed-width font) on the
	// screen as it is rendered, and each cell has its own coordinate, starting from 0.
	// Tabs are counted as [1..mTabSize] count empty spaces, depending on
	// how many space is necessary to reach the next tab stop.
	// For example, coordinate (1, 5) represents the character 'B' in a line "\tABC", when mTabSize = 4,
	// because it is rendered as "    ABC" on the screen.
	struct Coordinates {
		int mLine, mColumn;
		Coordinates() : mLine(0), mColumn(0) {
		}
		Coordinates(int aLine, int aColumn) : mLine(aLine), mColumn(aColumn) {
			assert(aLine >= 0);
			assert(aColumn >= 0);
		}
		static Coordinates Invalid() {
			static Coordinates invalid(-1, -1);
			return invalid;
		}

		bool operator==(const Coordinates &o) const {
			return mLine == o.mLine && mColumn == o.mColumn;
		}

		bool operator!=(const Coordinates &o) const {
			return mLine != o.mLine || mColumn != o.mColumn;
		}

		bool operator<(const Coordinates &o) const {
			if (mLine != o.mLine)
				return mLine < o.mLine;
			return mColumn < o.mColumn;
		}

		bool operator>(const Coordinates &o) const {
			if (mLine != o.mLine)
				return mLine > o.mLine;
			return mColumn > o.mColumn;
		}

		bool operator<=(const Coordinates &o) const {
			if (mLine != o.mLine)
				return mLine < o.mLine;
			return mColumn <= o.mColumn;
		}

		bool operator>=(const Coordinates &o) const {
			if (mLine != o.mLine)
				return mLine > o.mLine;
			return mColumn >= o.mColumn;
		}
	};

	struct Identifier {
		Coordinates mLocation;
		core::String mDeclaration;
	};

	typedef core::String String;
	typedef core::StringMap<Identifier> Identifiers;
	typedef core::StringSet Keywords;
	typedef core::Map<int, core::String> ErrorMarkers;
	typedef core::Set<int> Breakpoints;
	typedef core::Array<ImU32, (unsigned)PaletteIndex::Max> Palette;
	typedef uint8_t Char;

	struct Glyph {
		Char mChar;
		PaletteIndex mColorIndex = PaletteIndex::Default;
		bool mComment : 1;
		bool mMultiLineComment : 1;
		bool mPreprocessor : 1;

		Glyph(Char aChar, PaletteIndex aColorIndex)
			: mChar(aChar), mColorIndex(aColorIndex), mComment(false), mMultiLineComment(false), mPreprocessor(false) {
		}
	};

	typedef core::DynamicArray<Glyph> Line;
	typedef core::DynamicArray<Line> Lines;

	struct LanguageDefinition {
		typedef std::pair<core::String, PaletteIndex> TokenRegexString;
		typedef core::DynamicArray<TokenRegexString> TokenRegexStrings;
		typedef bool (*TokenizeCallback)(const char *in_begin, const char *in_end, const char *&out_begin,
										 const char *&out_end, PaletteIndex &paletteIndex);

		core::String mName;
		Keywords mKeywords;
		Identifiers mIdentifiers;
		Identifiers mPreprocIdentifiers;
		core::String mCommentStart, mCommentEnd, mSingleLineComment;
		char mPreprocChar;
		bool mAutoIndentation;

		TokenizeCallback mTokenize;

		TokenRegexStrings mTokenRegexStrings;

		bool mCaseSensitive;

		LanguageDefinition() : mPreprocChar('#'), mAutoIndentation(true), mTokenize(nullptr), mCaseSensitive(true) {
		}

		static const LanguageDefinition &CPlusPlus();
		static const LanguageDefinition &GLSL();
		static const LanguageDefinition &C();
		static const LanguageDefinition &Lua();
	};

	TextEditor();
	~TextEditor();

	void SetLanguageDefinition(const LanguageDefinition &aLanguageDef);
	const LanguageDefinition &GetLanguageDefinition() const {
		return _languageDefinition;
	}

	const Palette &GetPalette() const {
		return _paletteBase;
	}
	void SetPalette(const Palette &aValue);

	void SetErrorMarkers(const ErrorMarkers &aMarkers) {
		_errorMarkers = aMarkers;
	}
	void SetBreakpoints(const Breakpoints &aMarkers) {
		_breakpoints = aMarkers;
	}

	void Render(const char *aTitle, const ImVec2 &aSize = ImVec2(), bool aBorder = false);
	void SetText(const core::String &aText);
	core::String GetText() const;

	void SetTextLines(const core::DynamicArray<core::String> &aLines);
	core::DynamicArray<core::String> GetTextLines() const;

	core::String GetSelectedText() const;
	core::String GetCurrentLineText() const;

	int GetTotalLines() const {
		return (int)_lines.size();
	}
	bool IsOverwrite() const {
		return _overwrite;
	}

	void SetReadOnly(bool aValue);
	bool IsReadOnly() const {
		return _readOnly;
	}
	bool IsTextChanged() const {
		return _textChanged;
	}
	bool IsCursorPositionChanged() const {
		return _cursorPositionChanged;
	}

	bool IsColorizerEnabled() const {
		return _colorizerEnabled;
	}
	void SetColorizerEnable(bool aValue);

	Coordinates GetCursorPosition() const {
		return GetActualCursorCoordinates();
	}
	void SetCursorPosition(const Coordinates &aPosition);

	inline void SetHandleMouseInputs(bool aValue) {
		_handleMouseInputs = aValue;
	}
	inline bool IsHandleMouseInputsEnabled() const {
		return _handleKeyboardInputs;
	}

	inline void SetHandleKeyboardInputs(bool aValue) {
		_handleKeyboardInputs = aValue;
	}
	inline bool IsHandleKeyboardInputsEnabled() const {
		return _handleKeyboardInputs;
	}

	inline void SetImGuiChildIgnored(bool aValue) {
		_ignoreImGuiChild = aValue;
	}
	inline bool IsImGuiChildIgnored() const {
		return _ignoreImGuiChild;
	}

	inline void SetShowWhitespaces(bool aValue) {
		_showWhitespaces = aValue;
	}
	inline bool IsShowingWhitespaces() const {
		return _showWhitespaces;
	}

	void SetTabSize(int aValue);
	inline int GetTabSize() const {
		return _tabSize;
	}

	void InsertText(const core::String &aValue);
	void InsertText(const char *aValue);

	void MoveUp(int aAmount = 1, bool aSelect = false);
	void MoveDown(int aAmount = 1, bool aSelect = false);
	void MoveLeft(int aAmount = 1, bool aSelect = false, bool aWordMode = false);
	void MoveRight(int aAmount = 1, bool aSelect = false, bool aWordMode = false);
	void MoveTop(bool aSelect = false);
	void MoveBottom(bool aSelect = false);
	void MoveHome(bool aSelect = false);
	void MoveEnd(bool aSelect = false);

	void SetSelectionStart(const Coordinates &aPosition);
	void SetSelectionEnd(const Coordinates &aPosition);
	void SetSelection(const Coordinates &aStart, const Coordinates &aEnd, SelectionMode aMode = SelectionMode::Normal);
	void SelectWordUnderCursor();
	void SelectAll();
	bool HasSelection() const;

	void Copy();
	void Cut();
	void Paste();
	void Delete();

	bool CanUndo() const;
	bool CanRedo() const;
	void Undo(int aSteps = 1);
	void Redo(int aSteps = 1);

	static const Palette &GetDarkPalette();
	static const Palette &GetLightPalette();
	static const Palette &GetRetroBluePalette();

private:
	typedef core::DynamicArray<std::pair<std::regex, PaletteIndex>> RegexList;

	struct EditorState {
		Coordinates mSelectionStart;
		Coordinates mSelectionEnd;
		Coordinates mCursorPosition;
	};

	class UndoRecord {
	public:
		UndoRecord() {
		}
		~UndoRecord() {
		}

		UndoRecord(const core::String &aAdded, const TextEditor::Coordinates aAddedStart,
				   const TextEditor::Coordinates aAddedEnd,

				   const core::String &aRemoved, const TextEditor::Coordinates aRemovedStart,
				   const TextEditor::Coordinates aRemovedEnd,

				   TextEditor::EditorState &aBefore, TextEditor::EditorState &aAfter);

		void Undo(TextEditor *aEditor);
		void Redo(TextEditor *aEditor);

		core::String mAdded;
		Coordinates mAddedStart;
		Coordinates mAddedEnd;

		core::String mRemoved;
		Coordinates mRemovedStart;
		Coordinates mRemovedEnd;

		EditorState mBefore;
		EditorState mAfter;
	};

	typedef core::DynamicArray<UndoRecord> UndoBuffer;

	void ProcessInputs();
	void Colorize(int aFromLine = 0, int aCount = -1);
	void ColorizeRange(int aFromLine = 0, int aToLine = 0);
	void ColorizeInternal();
	float TextDistanceToLineStart(const Coordinates &aFrom) const;
	void EnsureCursorVisible();
	int GetPageSize() const;
	core::String GetText(const Coordinates &aStart, const Coordinates &aEnd) const;
	Coordinates GetActualCursorCoordinates() const;
	Coordinates SanitizeCoordinates(const Coordinates &aValue) const;
	void Advance(Coordinates &aCoordinates) const;
	void DeleteRange(const Coordinates &aStart, const Coordinates &aEnd);
	int InsertTextAt(Coordinates &aWhere, const char *aValue);
	void AddUndo(UndoRecord &aValue);
	Coordinates ScreenPosToCoordinates(const ImVec2 &aPosition) const;
	Coordinates FindWordStart(const Coordinates &aFrom) const;
	Coordinates FindWordEnd(const Coordinates &aFrom) const;
	Coordinates FindNextWord(const Coordinates &aFrom) const;
	int GetCharacterIndex(const Coordinates &aCoordinates) const;
	int GetCharacterColumn(int aLine, int aIndex) const;
	int GetLineCharacterCount(int aLine) const;
	int GetLineMaxColumn(int aLine) const;
	bool IsOnWordBoundary(const Coordinates &aAt) const;
	void RemoveLine(int aStart, int aEnd);
	void RemoveLine(int aIndex);
	Line &InsertLine(int aIndex);
	void EnterCharacter(ImWchar aChar, bool aShift);
	void Backspace();
	void DeleteSelection();
	core::String GetWordUnderCursor() const;
	core::String GetWordAt(const Coordinates &aCoords) const;
	ImU32 GetGlyphColor(const Glyph &aGlyph) const;

	void HandleKeyboardInputs();
	void HandleMouseInputs();
	void Render();

	float _lineSpacing = 1.0f;
	Lines _lines;
	EditorState _state;
	UndoBuffer _undoBuffer;
	int _undoIndex = 0;
	int _tabSize = 4;
	uint64_t _startTime;
	float _lastClick = -1.0f;
	/** position (in pixels) where a code line starts relative to the left of the TextEditor. */
	float _textStart = 20.0f;
	int _leftMargin = 10;
	int _colorRangeMin = 0;
	int _colorRangeMax = 0;
	SelectionMode _selectionMode = SelectionMode::Normal;

	bool _overwrite = false;
	bool _readOnly = false;
	bool _withinRender = false;
	bool _scrollToCursor = false;
	bool _scrollToTop = false;
	bool _textChanged = false;
	bool _colorizerEnabled = true;
	bool _handleKeyboardInputs = true;
	bool _handleMouseInputs = true;
	bool _ignoreImGuiChild = false;
	bool _showWhitespaces = true;
	bool _checkComments = true;
	bool _cursorPositionChanged = false;

	Palette _paletteBase;
	Palette _palette;
	LanguageDefinition _languageDefinition;
	RegexList _regexList;

	Breakpoints _breakpoints;
	ErrorMarkers _errorMarkers;
	ImVec2 _charAdvance;
	Coordinates _interactiveStart;
	Coordinates _interactiveEnd;
	core::String _lineBuffer;
};
