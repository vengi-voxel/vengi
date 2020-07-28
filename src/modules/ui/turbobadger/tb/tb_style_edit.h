/**
 * @file
 */

#pragma once

#include "tb_core.h"
#include "tb_linklist.h"
#include "tb_list.h"
#include "tb_widgets_common.h"

namespace tb {

class TBStyleEdit;
class TBBlock;
class TBTextFragment;
class TBTextFragmentContent;
class TBTextFragmentContentFactory;

/** Listener for TBStyleEdit. Implement in the enviorment the TBStyleEdit should render its content. */
class TBStyleEditListener {
public:
	virtual ~TBStyleEditListener() {
	}

	virtual void onChange(){};
	virtual bool onEnter() {
		return false;
	};
	virtual void invalidate(const TBRect &rect) = 0;
	virtual void drawString(int32_t x, int32_t y, TBFontFace *font, const TBColor &color, const char *str,
							int32_t len) = 0;
	virtual void drawRect(const TBRect &rect, const TBColor &color) = 0;
	virtual void drawRectFill(const TBRect &rect, const TBColor &color) = 0;
	virtual void drawTextSelectionBg(const TBRect &rect) = 0;
	virtual void drawContentSelectionFg(const TBRect &rect) = 0;
	virtual void drawCaret(const TBRect &rect) = 0;
	virtual void scroll(int32_t dx, int32_t dy) = 0;
	virtual void updateScrollbars() = 0;
	virtual void caretBlinkStart() = 0;
	virtual void caretBlinkStop() = 0;
};

/** Creates TBTextFragmentContent if the sequence of text matches known content. */

class TBTextFragmentContentFactory {
public:
	virtual ~TBTextFragmentContentFactory() {
	}
	/** Should return then length of the text that represents content
		that can be created by this factory, or 0 there's no match with any content.

		F.ex if we can create content for @c &lt;u&gt; it should return 3 if that is the beginning of
		text. That length will be consumed from the text output for the created content. */
	virtual int getContent(const char *text);

	/** Create content for a string previosly consumed by calling GetContent. */
	virtual TBTextFragmentContent *createFragmentContent(const char *text, int text_len);
};

class TBTextOfs {
public:
	TBTextOfs() : block(nullptr), ofs(0) {
	}
	TBTextOfs(TBBlock *block, int32_t ofs) : block(block), ofs(ofs) {
	}

	void set(TBBlock *newBlock, int32_t newOfs) {
		block = newBlock;
		ofs = newOfs;
	}
	void set(const TBTextOfs &pos) {
		block = pos.block;
		ofs = pos.ofs;
	}

	int32_t getGlobalOfs(TBStyleEdit *se) const;
	bool setGlobalOfs(TBStyleEdit *se, int32_t gofs);

public:
	TBBlock *block;
	int32_t ofs;
};

/** Handles the selected text in a TBStyleEdit. */

class TBSelection {
public:
	TBSelection(TBStyleEdit *styledit);
	void invalidate() const;
	void select(const TBTextOfs &newStart, const TBTextOfs &newStop);
	void select(const TBPoint &from, const TBPoint &to);
	void select(int globOfsFrom, int globOfsTo);
	void selectToCaret(TBBlock *oldCaretBlock, int32_t oldCaretOfs);
	void selectAll();
	void selectNothing();
	void correctOrder();
	void copyToClipboard();
	bool isBlockSelected(const TBBlock *block) const;
	bool isFragmentSelected(const TBBlock *block, TBTextFragment *elm) const;
	bool isSelected() const;
	void removeContent();
	bool getText(TBStr &text) const;

public:
	TBStyleEdit *styledit;
	TBTextOfs start, stop;
};

enum TB_CARET_POS { TB_CARET_POS_BEGINNING, TB_CARET_POS_END };

/** The caret in a TBStyleEdit. */
class TBCaret {
public:
	TBCaret(TBStyleEdit *styledit);
	void invalidate();
	void updatePos();
	bool move(bool forward, bool word);
	bool place(const TBPoint &point);
	bool place(TBBlock *block, int ofs, bool allowSnap = true, bool snapForward = false);
	void place(TB_CARET_POS pos);
	void avoidLineBreak();
	void paint(int32_t translateX, int32_t translateY);
	void resetBlink();
	void updateWantedX();

	int32_t getGlobalOfs() const {
		return pos.getGlobalOfs(styledit);
	}
	void setGlobalOfs(int32_t gofs, bool allowSnap = true, bool snapForward = false);

	TBTextFragment *getFragment();

private:
	void switchBlock(bool second);

public:
	TBStyleEdit *styledit;
	int32_t x, y; ///< Relative to the styledit
	int32_t width;
	int32_t height;
	int32_t wanted_x;
	bool on;
	bool prefer_first;
	TBTextOfs pos;
};

/** TBTextProps is a stack of properties used during layout & paint of TBStyleEdit. */

class TBTextProps {
public:
	class Data {
	public:
		TBFontDescription font_desc;
		TBColor text_color;
		bool underline = false;
	};
	TBTextProps() {
	}
	~TBTextProps() {
		while (list.getNumItems() > 0) {
			delete list.get(0);
			list.remove(0);
		}
	}

	void reset(const TBFontDescription &fontDesc, const TBColor &textColor);
	Data *push();
	void pop();

	/** Get the font face from the current font description. */
	TBFontFace *getFont() const;

public:
	int next_index = 0;
	TBListOf<Data> list;
	Data base;
	Data *data = nullptr;
};

/** TBPaintProps holds paint related data during paint of TBStyleEdit. */

class TBPaintProps {
public:
	TBBlock *block;
	TBTextProps *props;
	int32_t translate_x;
	int32_t translate_y;
};

/** A block of text (a line, that might be wrapped) */

class TBBlock : public TBLinkOf<TBBlock> {
public:
	TBBlock(TBStyleEdit *styledit);
	~TBBlock();

	void clear();
	void set(const char *newstr, int32_t len);
	void setAlign(TB_TEXT_ALIGN align);

	int32_t insertText(int32_t ofs, const char *text, int32_t len, bool allow_line_recurse);
	void removeContent(int32_t ofs, int32_t len);

	/** Check if this block contains extra line breaks and split into new blocks if it does. */
	void split();

	/** Check if we've lost the ending break on this block and if so merge it with the next block. */
	void merge();

	/** Layout the block. To be called when the text has changed or the layout width has changed.
		@param updateFragments Should be true if the text has been changed (will recreate elements).
		@param propagateHeight If true, all following blocks will be moved if the height changed. */
	void layout(bool updateFragments, bool propagateHeight);

	/** Update the size of this block. If propagate_height is true, all following blocks will be
		moved if the height changed. */
	void setSize(int32_t oldW, int32_t newW, int32_t newH, bool propagateHeight);

	TBTextFragment *findFragment(int32_t ofs, bool prefer_first = false) const;
	TBTextFragment *findFragment(int32_t x, int32_t y) const;

	int32_t calculateStringWidth(TBFontFace *font, const char *str, int len) const;
	int32_t calculateTabWidth(TBFontFace *font, int32_t xpos) const;
	int32_t calculateLineHeight(TBFontFace *font) const;
	int32_t calculateBaseline(TBFontFace *font) const;

	void invalidate() const;
	void buildSelectionRegion(int32_t translateX, int32_t translateY, TBTextProps *props, TBRegion &bgRegion,
							  TBRegion &fgRegion);
	void paint(int32_t translateX, int32_t translateY, TBTextProps *props);

public:
	TBStyleEdit *styledit;
	TBLinkListOf<TBTextFragment> fragments;

	int32_t ypos;
	int16_t height;
	int8_t align;
	int line_width_max;

	TBStr str;
	int32_t str_len;
	uint32_t syntax_data; ///< Free to use in any way from TBSyntaxHighlighter subclasses

private:
	int getStartIndentation(TBFontFace *font, int first_line_len) const;
};

/** Event in the TBUndoRedoStack. Each insert or remove change is stored as a TBUndoEvent, but they may also be merged
 * when appropriate. */

class TBUndoEvent {
public:
	int32_t gofs;
	TBStr text;
	bool insert;
};

/** Keeps track of all TBUndoEvents used for undo and redo functionality. */

class TBUndoRedoStack {
public:
	TBUndoRedoStack() : applying(false) {
	}
	~TBUndoRedoStack();

	void undo(TBStyleEdit *styledit);
	void redo(TBStyleEdit *styledit);
	void clear(bool clearUndo, bool clearRedo);

	TBUndoEvent *commit(TBStyleEdit *styledit, int32_t gofs, int32_t len, const char *text, bool insert);

public:
	TBListOf<TBUndoEvent> undos;
	TBListOf<TBUndoEvent> redos;
	bool applying;

private:
	void apply(TBStyleEdit *styledit, TBUndoEvent *e, bool reverse);
};

/** TBSyntaxHighlighter can be subclassed to give syntax highlighting on TBStyleEdit
	without altering the text (without inserting style markup) */
class TBSyntaxHighlighter {
public:
	virtual ~TBSyntaxHighlighter() {
	}

	/** Called when all fragments has been updated in the given block and syntax info should
		be updated. syntax_data can be stored in TBBlock and TBTextFragment */
	virtual void onFragmentsUpdated(TBBlock *block) {
	}

	/** Called after any change in TBStyleEdit when all blocks that changed have been updated. */
	virtual void onChange(TBStyleEdit *styledit) {
	}

	/** Called before painting each block */
	virtual void onPaintBlock(const TBPaintProps *props) {
	}

	/** Called before painting each fragment */
	virtual void onBeforePaintFragment(const TBPaintProps *props, TBTextFragment *fragment) {
	}

	/** Called after painting each fragment */
	virtual void onAfterPaintFragment(const TBPaintProps *props, TBTextFragment *fragment) {
	}
};

/** The textfragment baseclass for TBStyleEdit.

	TODO: This object is allocated on vast amounts and need
		  to shrink in size. Remove all cached positioning
		  and implement a fragment traverser (for TBBlock).
		  Also allocate fragments in chunks. */

class TBTextFragment : public TBLinkOf<TBTextFragment> {
public:
	TBTextFragment(TBTextFragmentContent *content = nullptr)
		: xpos(0), ypos(0), ofs(0), len(0), line_ypos(0), line_height(0), m_packed_init(0), content(content) {
	}
	~TBTextFragment();

	void init(const TBBlock *block, uint16_t ofs, uint16_t len);

	void updateContentPos(const TBBlock *block);

	void buildSelectionRegion(const TBPaintProps *props, TBRegion &bgRegion, TBRegion &fgRegion);
	void paint(const TBPaintProps *props);
	void click(const TBBlock *block, int button, uint32_t modifierkeys);

	bool isText() const {
		return !isEmbedded();
	}
	bool isEmbedded() const {
		return content != nullptr;
	}
	bool isBreak() const {
		return m_packed.is_break != 0;
	}
	bool isSpace() const {
		return m_packed.is_space != 0;
	}
	bool isTab() const {
		return m_packed.is_tab != 0;
	}

	int32_t getCharX(const TBBlock *block, TBFontFace *font, int32_t ofs);
	int32_t getCharOfs(const TBBlock *block, TBFontFace *font, int32_t x);

	/** Get the stringwidth. Handles passwordmode, tab, linebreaks etc automatically. */
	int32_t getStringWidth(const TBBlock *block, TBFontFace *font, const char *str, int len);

	bool getAllowBreakBefore(const TBBlock *block) const;
	bool getAllowBreakAfter(const TBBlock *block) const;

	const char *str(const TBBlock *block) const {
		return block->str.c_str() + ofs;
	}

	int32_t getWidth(const TBBlock *block, TBFontFace *font);
	int32_t getHeight(const TBBlock *block, TBFontFace *font);
	int32_t getBaseline(const TBBlock *block, TBFontFace *font);

public:
	int16_t xpos, ypos;
	uint16_t ofs, len;
	uint16_t line_ypos, line_height;
	union {
		struct {
			uint32_t is_break : 1;		 ///< Fragment is hard line break
			uint32_t is_space : 1;		 ///< Fragment is white space
			uint32_t is_tab : 1;		 ///< Fragment is tab
			uint32_t syntax_data : 10;   ///< Free to use in any way from TBSyntaxHighlighter subclasses
			uint32_t width : 11;		 ///< width cache. Bit number need to match shift in WIDTH_CACHE_MASK.
			uint32_t is_width_valid : 1; ///< width cache is set
		} m_packed;
		uint32_t m_packed_init;
	};
	static const uint32_t WIDTH_CACHE_MASK = (1 << 11) - 1;
	TBTextFragmentContent *content;
};

/** Edit and formats TBTextFragment's. It handles the text in a TBStyleEditView. */

class TBStyleEdit {
public:
	TBStyleEdit();
	virtual ~TBStyleEdit();

	void setListener(TBStyleEditListener *listener);
	void setContentFactory(TBTextFragmentContentFactory *content_factory);
	void setSyntaxHighlighter(TBSyntaxHighlighter *syntax_highlighter);

	void setFont(const TBFontDescription &font_desc);

	void paint(const TBRect &rect, const TBFontDescription &fontDesc, const TBColor &textColor);
	bool keyDown(int key, SPECIAL_KEY special_key, MODIFIER_KEYS modifierkeys);
	bool mouseDown(const TBPoint &point, int button, int clicks, MODIFIER_KEYS modifierkeys, bool touch);
	bool mouseUp(const TBPoint &point, int button, MODIFIER_KEYS modifierkeys, bool touch);
	bool mouseMove(const TBPoint &point);
	void focus(bool focus);

	void clear(bool init_new = true);
	bool setText(const char *text, TB_CARET_POS pos = TB_CARET_POS_BEGINNING);
	bool setText(const char *text, int text_len, TB_CARET_POS pos = TB_CARET_POS_BEGINNING);
	bool getText(TBStr &text);
	bool isEmpty() const;

	/** Set the default text alignment and all currently selected blocks,
		or the block of the current caret position if nothing is selected. */
	void setAlign(TB_TEXT_ALIGN align);
	void setMultiline(bool multiline = true);
	void setStyling(bool styling = true);
	void setReadOnly(bool readonly = true);
	void setSelection(bool selection = true);
	void setPassword(bool password = true);
	void setWrapping(bool wrapping = true);

	void cut();
	void copy();
	void paste();
	void del();

	void undo();
	void redo();
	bool canUndo() const {
		return undoredo.undos.getNumItems() != 0;
	}
	bool canRedo() const {
		return undoredo.redos.getNumItems() != 0;
	}

	void insertText(const char *text, bool afterLast = false,
					bool clearUndoRedo = false);
	void appendText(const char *text, bool clearUndoRedo = false) {
		insertText(text, true, clearUndoRedo);
	}
	void insertBreak();

	TBBlock *findBlock(int32_t y) const;

	void scrollIfNeeded(bool x = true, bool y = true);
	void setScrollPos(int32_t x, int32_t y);
	void setLayoutSize(int32_t width, int32_t height, bool is_virtual_reformat);
	void reformat(bool update_fragments);

	int32_t getContentWidth();
	int32_t getContentHeight() const;

	int32_t getOverflowX() const {
		return Max(content_width - layout_width, 0);
	}
	int32_t getOverflowY() const {
		return Max(content_height - layout_height, 0);
	}

public:
	TBStyleEditListener *listener;
	TBTextFragmentContentFactory default_content_factory;
	TBTextFragmentContentFactory *content_factory;
	TBSyntaxHighlighter *syntax_highlighter;
	int32_t layout_width;
	int32_t layout_height;
	int32_t content_width;
	int32_t content_height;

	TBLinkListOf<TBBlock> blocks;

	TBCaret caret;
	TBSelection selection;
	TBUndoRedoStack undoredo;
	TBTextProps text_props;

	int32_t scroll_x;
	int32_t scroll_y;

	int8_t select_state;
	TBPoint mousedown_point;
	TBTextFragment *mousedown_fragment;

	/** DEPRECATED! This will be removed when using different fonts is properly supported! */
	TBFontFace *font;
	TBFontDescription font_desc;

	TB_TEXT_ALIGN align;
	union {
		struct {
			uint32_t multiline_on : 1;
			uint32_t styling_on : 1;
			uint32_t read_only : 1;
			uint32_t selection_on : 1;
			uint32_t show_whitespace : 1;
			uint32_t password_on : 1;
			uint32_t wrapping : 1;
			uint32_t calculate_content_width_needed : 1; ///< If content_width needs to be updated next GetContentWidth-
			uint32_t lock_scrollbars_counter : 5; ///< Incremental counter for if UpdateScrollbar should be prohibited.
		} packed;
		uint32_t packed_init;
	};

	/** Call BeginLockScrollbars & EndLockScrollbars around a scope which does lots of changes,
		to prevent UpdateScrollbar from happening for each block (May cause recalculation of
		content_width by iterating through all blocks) */
	void beginLockScrollbars();
	void endLockScrollbars();

	/** Return true if changing layout_width and layout_height requires relayouting. */
	bool getSizeAffectsLayout() const;

	void invokeOnChange();
};

} // namespace tb
