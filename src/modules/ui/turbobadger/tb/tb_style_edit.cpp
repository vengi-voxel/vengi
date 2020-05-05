/**
 * @file
 */

#include "tb_style_edit.h"
#include "core/Assert.h"
#include "tb_font_renderer.h"
#include "tb_style_edit_content.h"
#include "tb_system.h"
#include "tb_tempbuffer.h"
#include "tb_widgets_common.h"
#include "utf8/utf8.h"

namespace tb {

#if 0 // Enable for some graphical debugging
#define TMPDEBUG(expr) expr
#define nTMPDEBUG(expr)
#else
#define TMPDEBUG(expr)
#define nTMPDEBUG(expr) expr
#endif

const int TAB_SPACE = 4;

const char *special_char_newln = "¶";	// 00B6 PILCROW SIGN
const char *special_char_space = "·";	// 00B7 MIDDLE DOT
const char *special_char_tab = "»";		 // 00BB RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
const char *special_char_password = "•"; // 2022 BULLET

static bool is_space(int8_t c) {
	switch (c) {
	case ' ':
		return true;
	}
	return false;
}

static bool is_linebreak(int8_t c) {
	switch (c) {
	case '\n':
	case '\r':
	case 0:
		return true;
	}
	return false;
}

static bool is_wordbreak(int8_t c) {
	switch (c) {
	case 0:
	case '\n':
	case '\r':
	case '\t':
	case '\"':
	case '\'':
	case '/':
	case '\\':
	case '[':
	case ']':
	case '{':
	case '}':
	case '(':
	case ')':
	case '>':
	case '<':
	case '-':
	case '+':
	case '*':
	case ',':
	case '.':
	case ';':
	case ':':
	case '&':
	case '|':
	case '#':
	case '!':
	case '=':
	case '^':
	case '~':
	case '?':
	case '@':
	case '$':
		return true;
	}
	return is_space(c);
}

/** Check if no line wrapping is allowed before the character at the given offset.
	The string must be null terminated. */
static bool is_never_break_before(const char *str, int ofs) {
	switch (str[ofs]) {
	case '\n':
	case '\r':
	case ' ':
	case '-':
	case '.':
	case ',':
	case ':':
	case ';':
	case '!':
	case '?':
	case ')':
	case ']':
	case '}':
	case '>':
		return true;
	case '\'':
	case '"':
		// Simple test if it's the first quote in a word surrounded by space.
		return ofs > 0 && !is_space(str[ofs - 1]);
	default:
		return false;
	}
}

/** Check if no line wrapping is allowed after the character at the given offset.
	The string must be null terminated. */
static bool is_never_break_after(const char *str, int ofs) {
	switch (str[ofs]) {
	case '(':
	case '[':
	case '{':
	case '<':
	case '@':
	case '$':
		return true;
	case '\'':
	case '"':
		// Simple test if it's the last quote in a word surrounded by space.
		return !is_space(str[ofs + 1]);
	default:
		return false;
	}
}

static bool getNextFragment(const char *text, TBTextFragmentContentFactory *contentFactory, int *fragLen,
							bool *isEmbed) {
	if (text[0] == '\t') {
		*fragLen = 1;
		return text[1] != 0;
	}
	if (text[0] == 0) // happens when not setting text and maby when setting ""
	{
		*fragLen = 0;
		return false;
	}
	if (text[0] == '\r' || text[0] == '\n') {
		int len = (text[0] == '\r' && text[1] == '\n') ? 2 : 1;
		*fragLen = len;
		return false;
	}
	if (contentFactory != nullptr) {
		if (int content_len = contentFactory->getContent(text)) {
			*fragLen = content_len;
			*isEmbed = true;
			return text[content_len] != 0;
		}
	}
	int i = 0;
	while (!is_wordbreak(text[i])) {
		i++;
	}
	if (i == 0) {
		if (is_wordbreak(text[i])) {
			i++;
		}
	}
	*fragLen = i;
	return text[i] != 0;
}

// == TBSelection ==================================================

TBSelection::TBSelection(TBStyleEdit *styledit) : styledit(styledit) {
}

void TBSelection::correctOrder() {
	if (start.block == stop.block && start.ofs == stop.ofs) {
		selectNothing();
	} else {
		if ((start.block == stop.block && start.ofs > stop.ofs) ||
			(start.block != stop.block && start.block->ypos > stop.block->ypos)) {
			TBTextOfs tmp = start;
			start = stop;
			stop = tmp;
		}
	}
}

void TBSelection::copyToClipboard() {
	if (isSelected()) {
		TBStr text;
		if (getText(text)) {
			TBClipboard::setText(text);
		}
	}
}

void TBSelection::invalidate() const {
	TBBlock *block = start.block;
	while (block != nullptr) {
		block->invalidate();
		if (block == stop.block) {
			break;
		}
		block = block->getNext();
	}
}

void TBSelection::select(const TBTextOfs &newStart, const TBTextOfs &newStop) {
	invalidate();
	start.set(newStart);
	stop.set(newStop);
	correctOrder();
	invalidate();
}

void TBSelection::select(const TBPoint &from, const TBPoint &to) {
	invalidate();
	styledit->caret.place(from);
	start.set(styledit->caret.pos);
	styledit->caret.place(to);
	stop.set(styledit->caret.pos);
	correctOrder();
	invalidate();
	styledit->caret.updateWantedX();
}

void TBSelection::select(int globOfsFrom, int globOfsTo) {
	TBTextOfs ofs1;
	TBTextOfs ofs2;
	ofs1.setGlobalOfs(styledit, globOfsFrom);
	ofs2.setGlobalOfs(styledit, globOfsTo);
	select(ofs1, ofs2);
}

void TBSelection::selectToCaret(TBBlock *oldCaretBlock, int32_t oldCaretOfs) {
	invalidate();
	if (start.block == nullptr) {
		start.set(oldCaretBlock, oldCaretOfs);
		stop.set(styledit->caret.pos);
	} else {
		if (start.block == oldCaretBlock && start.ofs == oldCaretOfs) {
			start.set(styledit->caret.pos);
		} else {
			stop.set(styledit->caret.pos);
		}
	}
	correctOrder();
	invalidate();
}

void TBSelection::selectAll() {
	start.set(styledit->blocks.getFirst(), 0);
	stop.set(styledit->blocks.getLast(), styledit->blocks.getLast()->str_len);
	invalidate();
}

void TBSelection::selectNothing() {
	invalidate();
	start.set(nullptr, 0);
	stop.set(nullptr, 0);
}

bool TBSelection::isBlockSelected(const TBBlock *block) const {
	if (!isSelected()) {
		return false;
	}
	return block->ypos >= start.block->ypos && block->ypos <= stop.block->ypos;
}

bool TBSelection::isFragmentSelected(const TBBlock *block, TBTextFragment *elm) const {
	if (!isSelected()) {
		return false;
	}
	if (start.block == stop.block) {
		if (block != start.block) {
			return false;
		}
		return start.ofs < elm->ofs + elm->len && stop.ofs >= elm->ofs;
	}
	if (block->ypos > start.block->ypos && block->ypos < stop.block->ypos) {
		return true;
	}
	if (block->ypos == start.block->ypos && elm->ofs + elm->len > start.ofs) {
		return true;
	}
	if (block->ypos == stop.block->ypos && elm->ofs < stop.ofs) {
		return true;
	}
	return false;
}

bool TBSelection::isSelected() const {
	return start.block != nullptr;
}

void TBSelection::removeContent() {
	if (!isSelected()) {
		return;
	}
	styledit->beginLockScrollbars();
	if (start.block == stop.block) {
		if (!styledit->undoredo.applying) {
			styledit->undoredo.commit(styledit, start.getGlobalOfs(styledit), stop.ofs - start.ofs,
									  start.block->str.c_str() + start.ofs, false);
		}
		start.block->removeContent(start.ofs, stop.ofs - start.ofs);
	} else {
		// Remove text in first block
		TBTempBuffer commit_string;
		int32_t start_gofs = 0;
		if (!styledit->undoredo.applying) {
			start_gofs = start.getGlobalOfs(styledit);
			commit_string.append(start.block->str.c_str() + start.ofs, start.block->str_len - start.ofs);
		}
		start.block->removeContent(start.ofs, start.block->str_len - start.ofs);

		// Remove text in all block in between start and stop
		TBBlock *block = start.block->getNext();
		while (block != stop.block) {
			if (!styledit->undoredo.applying) {
				commit_string.append(block->str, block->str_len);
			}

			TBBlock *next = block->getNext();
			styledit->blocks.doDelete(block);
			block = next;
		}

		// Remove text in last block
		if (!styledit->undoredo.applying) {
			commit_string.append(stop.block->str, stop.ofs);
			styledit->undoredo.commit(styledit, start_gofs, commit_string.getAppendPos(), commit_string.getData(),
									  false);
		}
		stop.block->removeContent(0, stop.ofs);
	}
	stop.block->merge();
	start.block->merge();
	styledit->caret.place(start.block, start.ofs);
	styledit->caret.updateWantedX();
	selectNothing();
	styledit->endLockScrollbars();
}

bool TBSelection::getText(TBStr &text) const {
	if (!isSelected()) {
		text.clear();
		return true;
	}
	if (start.block == stop.block) {
		text.append(start.block->str.c_str() + start.ofs, stop.ofs - start.ofs);
	} else {
		TBTempBuffer buf;
		buf.append(start.block->str.c_str() + start.ofs, start.block->str_len - start.ofs);
		TBBlock *block = start.block->getNext();
		while (block != stop.block) {
			buf.append(block->str, block->str_len);
			block = block->getNext();
		}
		// FIX: Add methods to change data owner from temp buffer to string!
		buf.append(stop.block->str, stop.ofs);
		text.set((char *)buf.getData(), buf.getAppendPos());
	}
	return true;
}

// == TBTextOfs =========================================================================

int32_t TBTextOfs::getGlobalOfs(TBStyleEdit *se) const {
	int32_t gofs = 0;
	TBBlock *b = se->blocks.getFirst();
	while ((b != nullptr) && b != block) {
		gofs += b->str_len;
		b = b->getNext();
	}
	gofs += ofs;
	return gofs;
}

bool TBTextOfs::setGlobalOfs(TBStyleEdit *se, int32_t gofs) {
	TBBlock *b = se->blocks.getFirst();
	while (b != nullptr) {
		int b_len = b->str_len;
		if (gofs <= b_len) {
			block = b;
			ofs = gofs;
			return true;
		}
		gofs -= b_len;
		b = b->getNext();
	}
	core_assert(!"out of range! not a valid global offset!");
	return false;
}

// == TBCaret ============================================================================

TBCaret::TBCaret(TBStyleEdit *styledit)
	: styledit(styledit), x(0), y(0), width(2), height(0), wanted_x(0), on(false), prefer_first(true) {
}

void TBCaret::invalidate() {
	if (styledit->listener != nullptr) {
		styledit->listener->invalidate(TBRect(x - styledit->scroll_x, y - styledit->scroll_y, width, height));
	}
}

void TBCaret::updatePos() {
	invalidate();
	TBTextFragment *fragment = getFragment();
	x = fragment->xpos + fragment->getCharX(pos.block, styledit->font, pos.ofs - fragment->ofs);
	y = fragment->ypos + pos.block->ypos;
	height = fragment->getHeight(pos.block, styledit->font);
	if (height == 0) {
		// If we don't have height, we're probably inside a style switch embed.
		y = fragment->line_ypos + pos.block->ypos;
		height = fragment->line_height;
	}
	invalidate();
}

bool TBCaret::move(bool forward, bool word) {
	// Make it stay on the same line if it reach the wrap point.
	prefer_first = forward;
	if (this->styledit->packed.password_on) {
		word = false;
	}

	int len = pos.block->str_len;
	if (word && !(forward && pos.ofs == len) && !(!forward && pos.ofs == 0)) {
		const char *str = pos.block->str;
		if (forward) {
			if (is_linebreak(str[pos.ofs])) {
				pos.ofs++;
			} else if (is_wordbreak(str[pos.ofs])) {
				while (pos.ofs < len && is_wordbreak(str[pos.ofs]) && !is_linebreak(str[pos.ofs])) {
					pos.ofs++;
				}
			} else {
				while (pos.ofs < len && !is_wordbreak(str[pos.ofs])) {
					pos.ofs++;
				}
				while (pos.ofs < len && is_space(str[pos.ofs])) {
					pos.ofs++;
				}
			}
		} else if (pos.ofs > 0) {
			while (pos.ofs > 0 && is_space(str[pos.ofs - 1])) {
				pos.ofs--;
			}
			if (pos.ofs > 0 && is_wordbreak(str[pos.ofs - 1])) {
				while (pos.ofs > 0 && is_wordbreak(str[pos.ofs - 1])) {
					pos.ofs--;
				}
			} else {
				while (pos.ofs > 0 && !is_wordbreak(str[pos.ofs - 1])) {
					pos.ofs--;
				}
			}
		}
	} else {
		if (forward && pos.ofs >= pos.block->str_len && (pos.block->getNext() != nullptr)) {
			pos.block = pos.block->getNext();
			pos.ofs = 0;
		} else if (!forward && pos.ofs <= 0 && (pos.block->prev != nullptr)) {
			pos.block = pos.block->getPrev();
			pos.ofs = pos.block->str_len;
		} else {
			int i = pos.ofs;
			if (forward) {
				utf8::move_inc(pos.block->str, &i, pos.block->str_len);
			} else {
				utf8::move_dec(pos.block->str, &i);
			}
			pos.ofs = i;
		}
	}
	return place(pos.block, pos.ofs, true, forward);
}

bool TBCaret::place(const TBPoint &point) {
	TBBlock *block = styledit->findBlock(point.y);
	TBTextFragment *fragment = block->findFragment(point.x, point.y - block->ypos);
	int ofs = fragment->ofs + fragment->getCharOfs(block, styledit->font, point.x - fragment->xpos);

	if (place(block, ofs)) {
		if (getFragment() != fragment) {
			prefer_first = !prefer_first;
			place(block, ofs);
		}
		return true;
	}
	return false;
}

void TBCaret::place(TB_CARET_POS pos) {
	if (pos == TB_CARET_POS_BEGINNING) {
		place(styledit->blocks.getFirst(), 0);
	} else if (pos == TB_CARET_POS_END) {
		place(styledit->blocks.getLast(), styledit->blocks.getLast()->str_len);
	}
}

bool TBCaret::place(TBBlock *block, int ofs, bool allowSnap, bool snapForward) {
	if (block != nullptr) {
		while ((block->getNext() != nullptr) && ofs > block->str_len) {
			ofs -= block->str_len;
			block = block->getNext();
		}
		while ((block->prev != nullptr) && ofs < 0) {
			block = block->getPrev();
			ofs += block->str_len;
		}
		if (ofs < 0) {
			ofs = 0;
		}
		if (ofs > block->str_len) {
			ofs = block->str_len;
		}

		// Avoid being inside linebreak
		if (allowSnap) {
			TBTextFragment *fragment = block->findFragment(ofs);
			if (ofs > fragment->ofs && fragment->isBreak()) {
				if (snapForward && (block->getNext() != nullptr)) {
					block = block->getNext();
					ofs = 0;
				} else {
					ofs = fragment->ofs;
				}
			}
		}
	}

	bool changed = (pos.block != block || pos.ofs != ofs);
	pos.set(block, ofs);

	if (block != nullptr) {
		updatePos();
	}

	return changed;
}

void TBCaret::avoidLineBreak() {
	TBTextFragment *fragment = getFragment();
	if (pos.ofs > fragment->ofs && fragment->isBreak()) {
		pos.ofs = fragment->ofs;
	}
	updatePos();
}

void TBCaret::paint(int32_t translateX, int32_t translateY) {
	//	if (on && !(styledit->select_state && styledit->selection.isSelected()))
	if (on || (styledit->select_state != 0)) {
		styledit->listener->drawCaret(TBRect(translateX + x, translateY + y, width, height));
	}
}

void TBCaret::resetBlink() {
	styledit->listener->caretBlinkStop();
	on = true;
	styledit->listener->caretBlinkStart();
}

void TBCaret::updateWantedX() {
	wanted_x = x;
}

TBTextFragment *TBCaret::getFragment() {
	return pos.block->findFragment(pos.ofs, prefer_first);
}

void TBCaret::switchBlock(bool second) {
}

void TBCaret::setGlobalOfs(int32_t gofs, bool allowSnap, bool snapForward) {
	TBTextOfs ofs;
	if (ofs.setGlobalOfs(styledit, gofs)) {
		place(ofs.block, ofs.ofs, allowSnap, snapForward);
	}
}

// == TBTextProps =======================================================================

void TBTextProps::reset(const TBFontDescription &fontDesc, const TBColor &textColor) {
	core_assert(next_index == 0);
	while (list.getNumItems() > 0) {
		delete list.get(0);
		list.remove(0);
	}
	next_index = 0;
	base.font_desc = fontDesc;
	base.text_color = textColor;
	base.underline = false;
	data = &base;
}

TBTextProps::Data *TBTextProps::push() {
	if (next_index >= list.getNumItems()) {
		Data *data = new Data;
		if (data == nullptr) {
			return nullptr;
		}
		if (!list.add(data)) {
			delete data;
			return nullptr;
		}
	}
	Data *next = list.get(next_index++);
	*next = *data;
	data = next;
	return data;
}

void TBTextProps::pop() {
	if (next_index == 0) {
		return; // Unballanced or we previously got OOM.
	}
	next_index--;
	data = next_index > 0 ? list.get(next_index - 1) : &base;
}

TBFontFace *TBTextProps::getFont() const {
	return g_font_manager->getFontFace(data->font_desc);
}

// ============================================================================

TBBlock::TBBlock(TBStyleEdit *styledit)
	: styledit(styledit), ypos(0), height(0), align(styledit->align), line_width_max(0), str_len(0), syntax_data(0) {
}

TBBlock::~TBBlock() {
	clear();
}

void TBBlock::clear() {
	fragments.deleteAll();
}

void TBBlock::set(const char *newstr, int32_t len) {
	str.set(newstr, len);
	str_len = len;
	split();
	layout(true, true);
}

void TBBlock::setAlign(TB_TEXT_ALIGN align) {
	if (this->align == align) {
		return;
	}
	this->align = align;
	layout(false, false);
}

int32_t TBBlock::insertText(int32_t ofs, const char *text, int32_t len, bool allowLineRecurse) {
	styledit->beginLockScrollbars();
	int first_line_len = len;
	for (int i = 0; i < len; i++) {
		if (text[i] == '\r' || text[i] == '\n') {
			first_line_len = i;
			// Include the line break too but not for single lines
			if (!styledit->packed.multiline_on) {
				break;
			}
			if (text[i] == '\r' && text[i + 1] == '\n') {
				first_line_len++;
			}
			first_line_len++;
			break;
		}
	}

	int32_t inserted_len = first_line_len;
	str.insert(ofs, text, first_line_len);
	str_len += first_line_len;

	split();
	layout(true, true);

	// Add the rest which was after the linebreak.
	if (allowLineRecurse && styledit->packed.multiline_on) {
		// Instead of recursively calling InsertText, we will loop through them all here
		TBBlock *next_block = getNext();
		const char *next_line_ptr = &text[first_line_len];
		int remaining = len - first_line_len;
		while (remaining > 0) {
			if (next_block == nullptr) {
				next_block = new TBBlock(styledit);
				styledit->blocks.addLast(next_block);
			}
			int consumed = next_block->insertText(0, next_line_ptr, remaining, false);
			next_line_ptr += consumed;
			inserted_len += consumed;
			remaining -= consumed;
			next_block = next_block->getNext();
		}
	}
	styledit->endLockScrollbars();
	return inserted_len;
}

void TBBlock::removeContent(int32_t ofs, int32_t len) {
	if (len == 0) {
		return;
	}
	str.remove(ofs, len);
	str_len -= len;
	layout(true, true);
}

void TBBlock::split() {
	int32_t len = str_len;
	int brlen =
		1; // FIX: skip ending newline fragment but not if there is several newlines and check for singleline newline.
	if (len > 1 && str.c_str()[len - 2] == '\r' && str.c_str()[len - 1] == '\n') {
		brlen++;
	}
	len -= brlen;
	for (int i = 0; i < len; i++) {
		if (is_linebreak(str.c_str()[i])) {
			TBBlock *block = new TBBlock(styledit);
			if (block == nullptr) {
				return;
			}
			styledit->blocks.addAfter(block, this);

			if (i < len - 1 && str.c_str()[i] == '\r' && str.c_str()[i + 1] == '\n') {
				i++;
			}
			i++;

			len = len + brlen - i;
			block->set(str.c_str() + i, len);
			str.remove(i, len);
			str_len -= len;
			break;
		}
	}
}

void TBBlock::merge() {
	TBBlock *next_block = getNext();
	if ((next_block != nullptr) && !fragments.getLast()->isBreak()) {
		str.append(getNext()->str);
		str_len = str.length();

		styledit->blocks.doDelete(next_block);

		height = 0; // Ensure that Layout propagate height to remaining blocks.
		layout(true, true);
	}
}

int32_t TBBlock::calculateTabWidth(TBFontFace *font, int32_t xpos) const {
	int tabsize = font->getStringWidth("x", 1) * TAB_SPACE;
	int p2 = int(xpos / tabsize) * tabsize + tabsize;
	return p2 - xpos;
}

int32_t TBBlock::calculateStringWidth(TBFontFace *font, const char *str, int len) const {
	if (styledit->packed.password_on) {
		// Convert the length in number or characters, since that's what matters for password width.
		len = utf8::count_characters(str, len);
		return font->getStringWidth(special_char_password) * len;
	}
	return font->getStringWidth(str, len);
}

int32_t TBBlock::calculateLineHeight(TBFontFace *font) const {
	return font->getHeight();
}

int32_t TBBlock::calculateBaseline(TBFontFace *font) const {
	return font->getAscent();
}

int TBBlock::getStartIndentation(TBFontFace *font, int firstLineLen) const {
	// Lines beginning with whitespace or list points, should
	// indent to the same as the beginning when wrapped.
	int indentation = 0;
	int i = 0;
	while (i < firstLineLen) {
		const char *current_str = str.c_str() + i;
		UCS4 uc = utf8::decode_next(str, &i, firstLineLen);
		switch (uc) {
		case '\t':
			indentation += calculateTabWidth(font, indentation);
			continue;
		case ' ':
		case '-':
		case '*':
			indentation += calculateStringWidth(font, current_str, 1);
			continue;
		case 0x2022: // BULLET
			indentation += calculateStringWidth(font, current_str, 3);
			continue;
		}
		break;
	}
	return indentation;
}

void TBBlock::layout(bool updateFragments, bool propagateHeight) {
	// Create fragments from the word fragments
	if (updateFragments || (fragments.getFirst() == nullptr)) {
		clear();

		int ofs = 0;
		const char *text = str;
		while (true) {
			int frag_len;
			bool is_embed = false;
			bool more = getNextFragment(&text[ofs], styledit->packed.styling_on ? styledit->content_factory : nullptr,
										&frag_len, &is_embed);

			TBTextFragment *fragment = new TBTextFragment();
			if (fragment == nullptr) {
				break;
			}

			fragment->init(this, ofs, frag_len);

			if (is_embed) {
				fragment->content = styledit->content_factory->createFragmentContent(&text[ofs], frag_len);
			}

			fragments.addLast(fragment);
			ofs += frag_len;

			if (!more) {
				break;
			}
		}
		if (styledit->syntax_highlighter != nullptr) {
			styledit->syntax_highlighter->onFragmentsUpdated(this);
		}
	}

	// Layout

	if (styledit->layout_width <= 0 && styledit->getSizeAffectsLayout()) {
		// Don't layout if we have no space. This will happen when setting text
		// before the widget has been layouted. We will relayout when we are resized.
		return;
	}

	int old_line_width_max = line_width_max;
	line_width_max = 0;
	int line_ypos = 0;
	int first_line_indentation = 0;
	TBTextFragment *first_fragment_on_line = fragments.getFirst();

	while (first_fragment_on_line != nullptr) {
		int line_width = 0;

		// Get the last fragment that should be laid out on the line while
		// calculating line width and preliminary x positions for the fragments.

		TBTextFragment *last_fragment_on_line = fragments.getLast();
		if (styledit->packed.wrapping) {
			// If we should wrap, search for the last allowed break point before the overflow.
			TBTextFragment *allowed_last_fragment = nullptr;

			int line_xpos = first_line_indentation;
			for (TBTextFragment *fragment = first_fragment_on_line; fragment != nullptr;
				 fragment = fragment->getNext()) {
				// Give the fragment the current x. Then tab widths are calculated properly in GetWidth.
				fragment->xpos = line_xpos;
				int fragment_w = fragment->getWidth(this, styledit->font);

				// Check if we overflow
				bool overflow = line_xpos + fragment_w > styledit->layout_width;

				if (overflow && (allowed_last_fragment != nullptr)) {
					last_fragment_on_line = allowed_last_fragment;
					break;
				}

				// Check if this is a allowed break position
				if (fragment->getAllowBreakAfter(this)) {
					if ((fragment->getNext() == nullptr) || fragment->getNext()->getAllowBreakBefore(this)) {
						allowed_last_fragment = fragment;
						line_width = line_xpos + fragment_w;
					}
				}

				line_xpos += fragment_w;
			}
			if (allowed_last_fragment == nullptr) {
				line_width = line_xpos;
			}
		} else {
			// When wrapping is off, just measure and set pos.
			line_width = first_line_indentation;
			for (TBTextFragment *fragment = first_fragment_on_line; fragment != nullptr;
				 fragment = fragment->getNext()) {
				fragment->xpos = line_width;
				line_width += fragment->getWidth(this, styledit->font);
			}
		}

		// Commit line - Layout each fragment on the line.

		int line_height = 0;
		int line_baseline = 0;
		TBTextFragment *fragment = first_fragment_on_line;
		while (fragment != nullptr) {
			line_height = Max(fragment->getHeight(this, styledit->font), line_height);
			line_baseline = Max(fragment->getBaseline(this, styledit->font), line_baseline);

			// These positions are not final. Will be adjusted below.
			fragment->ypos = line_ypos;

			if (fragment == last_fragment_on_line) {
				break;
			}
			fragment = fragment->getNext();
		}

		// Adjust the position of fragments on the line - now when we know the line totals.
		// x change because of alignment, y change because of fragment baseline vs line baseline.

		int32_t xofs = 0;
		if (align == TB_TEXT_ALIGN_RIGHT) {
			xofs = styledit->layout_width - line_width;
		} else if (align == TB_TEXT_ALIGN_CENTER) {
			xofs = (styledit->layout_width - line_width) / 2;
		}

		int adjusted_line_height = line_height;
		fragment = first_fragment_on_line;
		while (fragment != nullptr) {
			// The fragment need to know these later.
			fragment->line_ypos = line_ypos;
			fragment->line_height = line_height;

			// Adjust the position
			fragment->ypos += line_baseline - fragment->getBaseline(this, styledit->font);
			fragment->xpos += xofs;

			// We now know the final position so update content.
			fragment->updateContentPos(this);

			// Total line height may now have changed a bit.
			adjusted_line_height =
				Max(adjusted_line_height, line_baseline - fragment->getBaseline(this, styledit->font) +
											  fragment->getHeight(this, styledit->font));

			if (fragment == last_fragment_on_line) {
				break;
			}
			fragment = fragment->getNext();
		}

		// Update line_height set on fragments if needed
		if (line_height != adjusted_line_height) {
			for (fragment = first_fragment_on_line; fragment != last_fragment_on_line->getNext();
				 fragment = fragment->getNext()) {
				fragment->line_height = adjusted_line_height;
			}
		}

		line_width_max = Max(line_width_max, line_width);

		// This was the first line so calculate the indentation to use for the other lines.
		if (styledit->packed.wrapping && first_fragment_on_line == fragments.getFirst()) {
			first_line_indentation =
				getStartIndentation(styledit->font, last_fragment_on_line->ofs + last_fragment_on_line->len);
		}

		// Consume line

		line_ypos += adjusted_line_height;

		first_fragment_on_line = last_fragment_on_line->getNext();
	}

	ypos = getPrev() != nullptr ? getPrev()->ypos + getPrev()->height : 0;
	setSize(old_line_width_max, line_width_max, line_ypos, propagateHeight);

	invalidate();
}

void TBBlock::setSize(int32_t oldW, int32_t newW, int32_t newH, bool propagateHeight) {
	// Later: could optimize with Scroll here.
	int32_t dh = newH - height;
	height = newH;
	if (dh != 0 && propagateHeight) {
		TBBlock *block = getNext();
		while (block != nullptr) {
			block->ypos = block->getPrev()->ypos + block->getPrev()->height;
			block->invalidate();
			block = block->getNext();
		}
	}

	// Update content_width and content_height
	// content_width can only be calculated in constant time if we grow larger.
	// If we shrink our width and where equal to content_width, we don't know
	// how wide the widest block is and we set a flag to update it when needed.

	if (!styledit->packed.wrapping && !styledit->packed.multiline_on) {
		styledit->content_width = newW;
	} else if (newW > styledit->content_width) {
		styledit->content_width = newW;
	} else if (newW < oldW && oldW == styledit->content_width) {
		styledit->packed.calculate_content_width_needed = 1;
	}

	styledit->content_height = styledit->blocks.getLast()->ypos + styledit->blocks.getLast()->height;

	if ((styledit->listener != nullptr) && styledit->packed.lock_scrollbars_counter == 0 && propagateHeight) {
		styledit->listener->updateScrollbars();
	}
}

TBTextFragment *TBBlock::findFragment(int32_t ofs, bool preferFirst) const {
	TBTextFragment *fragment = fragments.getFirst();
	while (fragment != nullptr) {
		if (preferFirst && ofs <= fragment->ofs + fragment->len) {
			return fragment;
		}
		if (!preferFirst && ofs < fragment->ofs + fragment->len) {
			return fragment;
		}
		fragment = fragment->getNext();
	}
	return fragments.getLast();
}

TBTextFragment *TBBlock::findFragment(int32_t x, int32_t y) const {
	TBTextFragment *fragment = fragments.getFirst();
	while (fragment != nullptr) {
		if (y < fragment->line_ypos + fragment->line_height) {
			if (x < fragment->xpos + fragment->getWidth(this, styledit->font)) {
				return fragment;
			}
			if ((fragment->getNext() != nullptr) && fragment->getNext()->line_ypos > fragment->line_ypos) {
				return fragment;
			}
		}
		fragment = fragment->getNext();
	}
	return fragments.getLast();
}

void TBBlock::invalidate() const {
	if (styledit->listener != nullptr) {
		styledit->listener->invalidate(TBRect(0, -styledit->scroll_y + ypos, styledit->layout_width, height));
	}
}

void TBBlock::buildSelectionRegion(int32_t translateX, int32_t translateY, TBTextProps *props, TBRegion &bgRegion,
								   TBRegion &fgRegion) {
	if (!styledit->selection.isBlockSelected(this)) {
		return;
	}

	TBPaintProps paint_props;
	paint_props.block = this;
	paint_props.props = props;
	paint_props.translate_x = translateX;
	paint_props.translate_y = translateY + ypos;

	TBTextFragment *fragment = fragments.getFirst();
	while (fragment != nullptr) {
		fragment->buildSelectionRegion(&paint_props, bgRegion, fgRegion);
		fragment = fragment->getNext();
	}
}

void TBBlock::paint(int32_t translateX, int32_t translateY, TBTextProps *props) {
	TMPDEBUG(styledit->listener->DrawRect(TBRect(translate_x, translate_y + ypos, styledit->layout_width, height),
										  TBColor(255, 200, 0, 128)));

	TBPaintProps paint_props;
	paint_props.block = this;
	paint_props.props = props;
	paint_props.translate_x = translateX;
	paint_props.translate_y = translateY + ypos;

	if (styledit->syntax_highlighter != nullptr) {
		styledit->syntax_highlighter->onPaintBlock(&paint_props);
	}

	TBTextFragment *fragment = fragments.getFirst();
	while (fragment != nullptr) {
		if (styledit->syntax_highlighter != nullptr) {
			styledit->syntax_highlighter->onBeforePaintFragment(&paint_props, fragment);
		}

		fragment->paint(&paint_props);

		if (styledit->syntax_highlighter != nullptr) {
			styledit->syntax_highlighter->onAfterPaintFragment(&paint_props, fragment);
		}

		fragment = fragment->getNext();
	}
}

// == TBTextFragment =========================================================================

TBTextFragment::~TBTextFragment() {
	delete content;
}

void TBTextFragment::init(const TBBlock *block, uint16_t ofs, uint16_t len) {
	this->ofs = ofs;
	this->len = len;
	m_packed.is_break = str(block)[0] == '\r' || str(block)[0] == '\n';
	m_packed.is_space = is_space(str(block)[0]);
	m_packed.is_tab = str(block)[0] == '\t';
}

void TBTextFragment::updateContentPos(const TBBlock *block) {
	if (content != nullptr) {
		content->updatePos(block, xpos, ypos + block->ypos);
	}
}

void TBTextFragment::buildSelectionRegion(const TBPaintProps *props, TBRegion &bgRegion, TBRegion &fgRegion) {
	const TBBlock *block = props->block;
	if (!block->styledit->selection.isFragmentSelected(block, this)) {
		return;
	}

	const int x = props->translate_x + xpos;
	const int y = props->translate_y + ypos;
	TBFontFace *font = props->props->getFont();

	if (content != nullptr) {
		// Selected embedded content should add to the foreground region.
		fgRegion.includeRect(TBRect(x, y, getWidth(block, font), getHeight(block, font)));
		return;
	}

	// Selected text should add to the backgroud region.
	TBSelection *sel = &block->styledit->selection;

	int sofs1 = sel->start.block == block ? sel->start.ofs : 0;
	int sofs2 = sel->stop.block == block ? sel->stop.ofs : block->str_len;
	sofs1 = Max(sofs1, (int)ofs);
	sofs2 = Min(sofs2, (int)(ofs + len));

	int s1x = getStringWidth(block, font, block->str.c_str() + ofs, sofs1 - ofs);
	int s2x = getStringWidth(block, font, block->str.c_str() + sofs1, sofs2 - sofs1);

	bgRegion.includeRect(TBRect(x + s1x, y, s2x, getHeight(block, font)));
}

void TBTextFragment::paint(const TBPaintProps *props) {
	TBStyleEditListener *listener = props->block->styledit->listener;

	const int x = props->translate_x + xpos;
	const int y = props->translate_y + ypos;
	const TBColor color = props->props->data->text_color;
	TBFontFace *font = props->props->getFont();
	TBBlock *block = props->block;

	if (content != nullptr) {
		content->paint(props, this);
		return;
	}
	TMPDEBUG(
		listener->DrawRect(TBRect(x, y, getWidth(block, font), getHeight(block, font)), TBColor(255, 255, 255, 128)));

	if (block->styledit->packed.password_on) {
		int cw = block->calculateStringWidth(font, special_char_password);
		int num_char = utf8::count_characters(str(block), len);
		for (int i = 0; i < num_char; i++) {
			listener->drawString(x + i * cw, y, font, color, special_char_password);
		}
	} else if (block->styledit->packed.show_whitespace) {
		if (isTab()) {
			listener->drawString(x, y, font, color, special_char_tab);
		} else if (isBreak()) {
			listener->drawString(x, y, font, color, special_char_newln);
		} else if (isSpace()) {
			listener->drawString(x, y, font, color, special_char_space);
		} else {
			listener->drawString(x, y, font, color, str(block), len);
		}
	} else if (!isTab() && !isBreak() && !isSpace()) {
		listener->drawString(x, y, font, color, str(block), len);
	}

	if (props->props->data->underline) {
		int line_h = font->getHeight() / 16;
		line_h = Max(line_h, 1);
		listener->drawRectFill(TBRect(x, y + getBaseline(block, font) + 1, getWidth(block, font), line_h), color);
	}
}

void TBTextFragment::click(const TBBlock *block, int button, uint32_t modifierkeys) {
	if (content != nullptr) {
		content->click(block, this, button, modifierkeys);
	}
}

int32_t TBTextFragment::getWidth(const TBBlock *block, TBFontFace *font) {
	if (m_packed.is_width_valid) {
		return m_packed.width;
	}
	int32_t width = 0;
	if (content != nullptr) {
		width = content->getWidth(block, font, this);
	} else if (isBreak()) {
		width = 0;
	} else if (isTab()) {
		width = block->calculateTabWidth(font, xpos);
	} else {
		width = block->calculateStringWidth(font, block->str.c_str() + ofs, len);
	}
	if ((((uint32_t)width) & WIDTH_CACHE_MASK) == (uint32_t)width) {
		m_packed.is_width_valid = 1;
		m_packed.width = (uint32_t)width;
	}
	return width;
}

int32_t TBTextFragment::getHeight(const TBBlock *block, TBFontFace *font) {
	if (content != nullptr) {
		return content->getHeight(block, font, this);
	}
	return block->calculateLineHeight(font);
}

int32_t TBTextFragment::getBaseline(const TBBlock *block, TBFontFace *font) {
	if (content != nullptr) {
		return content->getBaseline(block, font, this);
	}
	return block->calculateBaseline(font);
}

int32_t TBTextFragment::getCharX(const TBBlock *block, TBFontFace *font, int32_t ofs) {
	core_assert(ofs >= 0 && ofs <= len);

	if (isEmbedded() || isTab()) {
		return ofs == 0 ? 0 : getWidth(block, font);
	}
	if (isBreak()) {
		return 0;
	}

	return block->calculateStringWidth(font, block->str.c_str() + this->ofs, ofs);
}

int32_t TBTextFragment::getCharOfs(const TBBlock *block, TBFontFace *font, int32_t x) {
	if (isEmbedded() || isTab()) {
		return x > getWidth(block, font) / 2 ? 1 : 0;
	}
	if (isBreak()) {
		return 0;
	}

	const char *str = block->str.c_str() + ofs;
	int i = 0;
	while (i < len) {
		int pos = i;
		utf8::move_inc(str, &i, len);
		int last_char_len = i - pos;
		// Always measure from the beginning of the fragment because of eventual kerning & text shaping etc.
		int width_except_last_char = block->calculateStringWidth(font, str, i - last_char_len);
		int width = block->calculateStringWidth(font, str, i);
		if (x < width - (width - width_except_last_char) / 2) {
			return pos;
		}
	}
	return len;
}

int32_t TBTextFragment::getStringWidth(const TBBlock *block, TBFontFace *font, const char *str, int len) {
	if (len == 0) {
		return 0;
	}
	if (len == this->len) {
		return getWidth(block, font);
	}
	if (isTab()) {
		return block->calculateTabWidth(font, xpos);
	}
	if (isBreak()) {
		return 8;
	}
	return block->calculateStringWidth(font, str, len);
}

bool TBTextFragment::getAllowBreakBefore(const TBBlock *block) const {
	if (content != nullptr) {
		return content->getAllowBreakBefore(block);
	}
	return (len != 0U) && !is_never_break_before(block->str.c_str(), ofs);
}

bool TBTextFragment::getAllowBreakAfter(const TBBlock *block) const {
	if (content != nullptr) {
		return content->getAllowBreakAfter(block);
	}
	return (len != 0U) && !is_never_break_after(block->str.c_str(), ofs + len - 1);
}

// ============================================================================

TBStyleEdit::TBStyleEdit()
	: listener(nullptr), content_factory(&default_content_factory), syntax_highlighter(nullptr), layout_width(0),
	  layout_height(0), content_width(0), content_height(0), caret(nullptr), selection(nullptr), scroll_x(0),
	  scroll_y(0), select_state(0), mousedown_fragment(nullptr), font(nullptr), align(TB_TEXT_ALIGN_LEFT),
	  packed_init(0) {
	caret.styledit = this;
	selection.styledit = this;
	TMPDEBUG(packed.show_whitespace = true);

	font_desc = g_font_manager->getDefaultFontDescription();
	font = g_font_manager->getFontFace(font_desc);

#ifdef TB_TARGET_WINDOWS
	packed.win_style_br = 1;
#endif
	packed.selection_on = 1;

	clear();
}

TBStyleEdit::~TBStyleEdit() {
	listener->caretBlinkStop();
	clear(false);
}

void TBStyleEdit::setListener(TBStyleEditListener *listener) {
	this->listener = listener;
}

void TBStyleEdit::setContentFactory(TBTextFragmentContentFactory *contentFactory) {
	if (contentFactory != nullptr) {
		this->content_factory = contentFactory;
	} else {
		this->content_factory = &default_content_factory;
	}
}

void TBStyleEdit::setSyntaxHighlighter(TBSyntaxHighlighter *syntaxHighlighter) {
	this->syntax_highlighter = syntaxHighlighter;
	reformat(true);
}

void TBStyleEdit::setFont(const TBFontDescription &fontDesc) {
	if (this->font_desc == fontDesc) {
		return;
	}
	this->font_desc = fontDesc;
	font = g_font_manager->getFontFace(fontDesc);
	reformat(true);
}

void TBStyleEdit::clear(bool initNew) {
	undoredo.clear(true, true);
	selection.selectNothing();

	if (initNew && (blocks.getFirst() != nullptr) && isEmpty()) {
		return;
	}

	for (TBBlock *block = blocks.getFirst(); block != nullptr; block = block->getNext()) {
		block->invalidate();
	}
	blocks.deleteAll();

	if (initNew) {
		blocks.addLast(new TBBlock(this));
		blocks.getFirst()->set("", 0);
	}

	caret.place(blocks.getFirst(), 0);
	caret.updateWantedX();
}

void TBStyleEdit::scrollIfNeeded(bool x, bool y) {
	if (layout_width <= 0 || layout_height <= 0) {
		return; // This is likely during construction before layout.
	}

	int32_t newx = scroll_x;
	int32_t newy = scroll_y;
	if (x) {
		if (caret.x - scroll_x < 0) {
			newx = caret.x;
		}
		if (caret.x + caret.width - scroll_x > layout_width) {
			newx = caret.x + caret.width - layout_width;
		}
	}
	if (y) {
		if (caret.y - scroll_y < 0) {
			newy = caret.y;
		}
		if (caret.y + caret.height - scroll_y > layout_height) {
			newy = caret.y + caret.height - layout_height;
		}
	}
	setScrollPos(newx, newy);
}

void TBStyleEdit::setScrollPos(int32_t x, int32_t y) {
	x = Min(x, getContentWidth() - layout_width);
	y = Min(y, getContentHeight() - layout_height);
	x = Max(x, 0);
	y = Max(y, 0);
	if (!packed.multiline_on) {
		y = 0;
	}
	int dx = scroll_x - x;
	int dy = scroll_y - y;
	if ((dx != 0) || (dy != 0)) {
		scroll_x = x;
		scroll_y = y;
		listener->scroll(dx, dy);
	}
}

void TBStyleEdit::beginLockScrollbars() {
	packed.lock_scrollbars_counter++;
}

void TBStyleEdit::endLockScrollbars() {
	packed.lock_scrollbars_counter--;
	if ((listener != nullptr) && packed.lock_scrollbars_counter == 0) {
		listener->updateScrollbars();
	}
}

void TBStyleEdit::setLayoutSize(int32_t width, int32_t height, bool isVirtualReformat) {
	if (width == layout_width && height == layout_height) {
		return;
	}

	bool doReformat = layout_width != width;
	layout_width = width;
	layout_height = height;

	if (doReformat && getSizeAffectsLayout()) {
		reformat(false);
	}

	caret.updatePos();
	caret.updateWantedX();

	if (!isVirtualReformat) {
		setScrollPos(scroll_x, scroll_y); ///< Trig a bounds check (scroll if outside)
	}
}

bool TBStyleEdit::getSizeAffectsLayout() const {
	return packed.wrapping || align != TB_TEXT_ALIGN_LEFT;
}

void TBStyleEdit::reformat(bool updateFragments) {
	int ypos = 0;
	beginLockScrollbars();
	TBBlock *block = blocks.getFirst();
	while (block != nullptr) {
		// Update ypos directly instead of using "propagate_height" since propagating
		// would iterate forward through all remaining blocks and we're going to visit
		// them all anyway.
		block->ypos = ypos;
		block->layout(updateFragments, false);
		ypos += block->height;
		block = block->getNext();
	}
	endLockScrollbars();
	listener->invalidate(TBRect(0, 0, layout_width, layout_height));
}

int32_t TBStyleEdit::getContentWidth() {
	if (packed.calculate_content_width_needed) {
		packed.calculate_content_width_needed = 0;
		content_width = 0;
		TBBlock *block = blocks.getFirst();
		while (block != nullptr) {
			content_width = Max(content_width, block->line_width_max);
			block = block->getNext();
		}
	}
	return content_width;
}

int32_t TBStyleEdit::getContentHeight() const {
	return content_height;
}

void TBStyleEdit::paint(const TBRect &rect, const TBFontDescription &fontDesc, const TBColor &textColor) {
	text_props.reset(fontDesc, textColor);

	// Find the first visible block
	TBBlock *first_visible_block = blocks.getFirst();
	while (first_visible_block != nullptr) {
		if (first_visible_block->ypos + first_visible_block->height - scroll_y >= 0) {
			break;
		}
		first_visible_block = first_visible_block->getNext();
	}

	// Get the selection region for all visible blocks
	TBRegion bg_region;
	TBRegion fg_region;
	if (selection.isSelected()) {
		TBBlock *block = first_visible_block;
		while (block != nullptr) {
			if (block->ypos - scroll_y > rect.y + rect.h) {
				break;
			}
			block->buildSelectionRegion(-scroll_x, -scroll_y, &text_props, bg_region, fg_region);
			block = block->getNext();
		}

		// Paint bg selection
		for (int i = 0; i < bg_region.getNumRects(); i++) {
			listener->drawTextSelectionBg(bg_region.getRect(i));
		}
	}

	// Paint the content
	TBBlock *block = first_visible_block;
	while (block != nullptr) {
		if (block->ypos - scroll_y > rect.y + rect.h) {
			break;
		}
		block->paint(-scroll_x, -scroll_y, &text_props);
		block = block->getNext();
	}

	// Paint fg selection
	for (int i = 0; i < fg_region.getNumRects(); i++) {
		listener->drawTextSelectionBg(fg_region.getRect(i));
	}

	// Paint caret
	caret.paint(-scroll_x, -scroll_y);
}

void TBStyleEdit::insertBreak() {
	if (!packed.multiline_on) {
		return;
	}

	const char *new_line_str = "\n";

	// If we stand at the end and don't have any ending break, we're standing at the last line and
	// should insert breaks twice. One to end the current line, and one for the new empty line.
	if (caret.pos.ofs == caret.pos.block->str_len && !caret.pos.block->fragments.getLast()->isBreak()) {
		new_line_str = "\n\n";
	}

	insertText(new_line_str);

	caret.avoidLineBreak();
	if (caret.pos.block->getNext() != nullptr) {
		caret.place(caret.pos.block->getNext(), 0);
	}
}

void TBStyleEdit::insertText(const char *text, int32_t len, bool afterLast, bool clearUndoRedo) {
	if (len == TB_ALL_TO_TERMINATION) {
		len = SDL_strlen(text);
	}

	selection.removeContent();

	if (afterLast) {
		caret.place(blocks.getLast(), blocks.getLast()->str_len, false);
	}

	int32_t len_inserted = caret.pos.block->insertText(caret.pos.ofs, text, len, true);
	if (clearUndoRedo) {
		undoredo.clear(true, true);
	} else {
		undoredo.commit(this, caret.getGlobalOfs(), len_inserted, text, true);
	}

	caret.place(caret.pos.block, caret.pos.ofs + len, false);
	caret.updatePos();
	caret.updateWantedX();
}

TBBlock *TBStyleEdit::findBlock(int32_t y) const {
	TBBlock *block = blocks.getFirst();
	while (block != nullptr) {
		if (y < block->ypos + block->height) {
			return block;
		}
		block = block->getNext();
	}
	return blocks.getLast();
}

bool TBStyleEdit::keyDown(int key, SPECIAL_KEY specialKey, MODIFIER_KEYS modifierkeys) {
	if (select_state != 0) {
		return false;
	}

	bool handled = true;
	bool move_caret = specialKey == TB_KEY_LEFT || specialKey == TB_KEY_RIGHT || specialKey == TB_KEY_UP ||
					  specialKey == TB_KEY_DOWN || specialKey == TB_KEY_HOME || specialKey == TB_KEY_END ||
					  specialKey == TB_KEY_PAGE_UP || specialKey == TB_KEY_PAGE_DOWN;

	if (((modifierkeys & TB_SHIFT) == 0U) && move_caret) {
		selection.selectNothing();
	}

	TBTextOfs old_caret_pos = caret.pos;
	TBTextFragment *old_caret_elm = caret.getFragment();

	if ((specialKey == TB_KEY_UP || specialKey == TB_KEY_DOWN) && ((modifierkeys & TB_CTRL) != 0U)) {
		int32_t line_height = old_caret_pos.block->calculateLineHeight(font);
		int32_t new_y = scroll_y + (specialKey == TB_KEY_UP ? -line_height : line_height);
		setScrollPos(scroll_x, new_y);
	} else if (specialKey == TB_KEY_LEFT) {
		caret.move(false, (modifierkeys & TB_CTRL) != 0U);
	} else if (specialKey == TB_KEY_RIGHT) {
		caret.move(true, (modifierkeys & TB_CTRL) != 0U);
	} else if (specialKey == TB_KEY_UP) {
		handled = caret.place(TBPoint(caret.wanted_x, old_caret_pos.block->ypos + old_caret_elm->line_ypos - 1));
	} else if (specialKey == TB_KEY_DOWN) {
		handled = caret.place(TBPoint(caret.wanted_x, old_caret_pos.block->ypos + old_caret_elm->line_ypos +
														  old_caret_elm->line_height + 1));
	} else if (specialKey == TB_KEY_PAGE_UP) {
		caret.place(TBPoint(caret.wanted_x, caret.y - layout_height));
	} else if (specialKey == TB_KEY_PAGE_DOWN) {
		caret.place(TBPoint(caret.wanted_x, caret.y + layout_height + old_caret_elm->line_height));
	} else if (specialKey == TB_KEY_HOME && ((modifierkeys & TB_CTRL) != 0U)) {
		caret.place(TBPoint(0, 0));
	} else if (specialKey == TB_KEY_END && ((modifierkeys & TB_CTRL) != 0U)) {
		caret.place(TBPoint(32000, blocks.getLast()->ypos + blocks.getLast()->height));
	} else if (specialKey == TB_KEY_HOME) {
		caret.place(TBPoint(0, caret.y));
	} else if (specialKey == TB_KEY_END) {
		caret.place(TBPoint(32000, caret.y));
	} else if (key == '8' && ((modifierkeys & TB_CTRL) != 0U)) {
		packed.show_whitespace = !packed.show_whitespace;
		listener->invalidate(TBRect(0, 0, layout_width, layout_height));
	} else if (!packed.read_only && (specialKey == TB_KEY_DELETE || specialKey == TB_KEY_BACKSPACE)) {
		if (!selection.isSelected()) {
			caret.move(specialKey == TB_KEY_DELETE, (modifierkeys & TB_CTRL) != 0U);
			selection.selectToCaret(old_caret_pos.block, old_caret_pos.ofs);
		}
		selection.removeContent();
	} else if (!packed.read_only && ((modifierkeys & TB_SHIFT) == 0U) &&
			   (specialKey == TB_KEY_TAB && packed.multiline_on)) {
		insertText("\t", 1);
	} else if (!packed.read_only && (specialKey == TB_KEY_ENTER && packed.multiline_on) &&
			   ((modifierkeys & TB_CTRL) == 0U)) {
		insertBreak();
	} else if (!packed.read_only && ((key != 0) && ((modifierkeys & TB_CTRL) == 0U)) && specialKey != TB_KEY_ENTER) {
		char utf8[8];
		int len = utf8::encode(key, utf8);
		insertText(utf8, len);
	} else {
		handled = false;
	}

	if (((modifierkeys & TB_SHIFT) != 0U) && move_caret) {
		selection.selectToCaret(old_caret_pos.block, old_caret_pos.ofs);
	}

	if (!(specialKey == TB_KEY_UP || specialKey == TB_KEY_DOWN || specialKey == TB_KEY_PAGE_UP ||
		  specialKey == TB_KEY_PAGE_DOWN)) {
		caret.updateWantedX();
	}

	caret.resetBlink();

	// Hooks
	if (!move_caret && handled) {
		invokeOnChange();
	}
	if (specialKey == TB_KEY_ENTER && ((modifierkeys & TB_CTRL) == 0U)) {
		if (listener->onEnter()) {
			handled = true;
		}
	}
	if (handled) {
		scrollIfNeeded();
	}

	return handled;
}

void TBStyleEdit::cut() {
	if (packed.password_on) {
		return;
	}
	copy();
	keyDown(0, TB_KEY_DELETE, TB_MODIFIER_NONE);
}

void TBStyleEdit::copy() {
	if (packed.password_on) {
		return;
	}
	selection.copyToClipboard();
}

void TBStyleEdit::paste() {
	TBStr text;
	if (TBClipboard::hasText() && TBClipboard::getText(text)) {
		insertText(text, text.length());
		scrollIfNeeded(true, true);
		invokeOnChange();
	}
}

void TBStyleEdit::del() {
	if (selection.isSelected()) {
		selection.removeContent();
		invokeOnChange();
	}
}

void TBStyleEdit::undo() {
	if (canUndo()) {
		undoredo.undo(this);
		invokeOnChange();
	}
}

void TBStyleEdit::redo() {
	if (canRedo()) {
		undoredo.redo(this);
		invokeOnChange();
	}
}

bool TBStyleEdit::mouseDown(const TBPoint &point, int button, int clicks, MODIFIER_KEYS modifierkeys, bool touch) {
	if (button != 1) {
		return false;
	}

	if (touch) {
		mousedown_point = TBPoint(point.x + scroll_x, point.y + scroll_y);
	} else if (packed.selection_on) {
		// if (modifierkeys & P_SHIFT) // Select to new caretpos
		//{
		//}
		// else // Start selection
		{
			mousedown_point = TBPoint(point.x + scroll_x, point.y + scroll_y);
			selection.selectNothing();

			// clicks is 1 to infinite, and here we support only doubleclick, so make it either single or double.
			select_state = ((clicks - 1) % 2) + 1;

			mouseMove(point);

			if (caret.pos.block != nullptr) {
				mousedown_fragment =
					caret.pos.block->findFragment(mousedown_point.x, mousedown_point.y - caret.pos.block->ypos);
			}
		}
		caret.resetBlink();
	}
	return true;
}

bool TBStyleEdit::mouseUp(const TBPoint &point, int button, MODIFIER_KEYS modifierkeys, bool touch) {
	if (button != 1) {
		return false;
	}

	if (touch && !TBWidget::cancel_click) {
		selection.selectNothing();
		caret.place(mousedown_point);
		caret.updateWantedX();
		caret.resetBlink();
	}

	select_state = 0;
	if ((caret.pos.block != nullptr) && !TBWidget::cancel_click) {
		TBTextFragment *fragment =
			caret.pos.block->findFragment(point.x + scroll_x, point.y + scroll_y - caret.pos.block->ypos);
		if ((fragment != nullptr) && fragment == mousedown_fragment) {
			fragment->click(caret.pos.block, button, modifierkeys);
		}
	}
	return true;
}

bool TBStyleEdit::mouseMove(const TBPoint &point) {
	if (select_state != 0) {
		TBPoint p(point.x + scroll_x, point.y + scroll_y);
		selection.select(mousedown_point, p);

		if (select_state == 2) {
			bool has_initial_selection = selection.isSelected();

			if (has_initial_selection) {
				caret.place(selection.start.block, selection.start.ofs);
			}
			caret.move(false, true);
			selection.start.set(caret.pos);

			if (has_initial_selection) {
				caret.place(selection.stop.block, selection.stop.ofs);
			}
			caret.move(true, true);
			selection.stop.set(caret.pos);

			selection.correctOrder();
			caret.updateWantedX();
		}
		return true;
	}
	return false;
}

void TBStyleEdit::focus(bool focus) {
	if (focus) {
		listener->caretBlinkStart();
	} else {
		listener->caretBlinkStop();
	}

	caret.on = focus;
	caret.invalidate();
	selection.invalidate();
}

bool TBStyleEdit::setText(const char *text, TB_CARET_POS pos) {
	return setText(text, SDL_strlen(text), pos);
}

bool TBStyleEdit::setText(const char *text, int textLen, TB_CARET_POS pos) {
	if ((text == nullptr) || (*text == 0)) {
		clear(true);
		caret.updateWantedX();
		scrollIfNeeded(true, true);
		return true;
	}

	clear(true);
	blocks.getFirst()->insertText(0, text, textLen, true);

	caret.place(blocks.getFirst(), 0);
	caret.updateWantedX();
	scrollIfNeeded(true, false);

	if (pos == TB_CARET_POS_END) {
		caret.place(blocks.getLast(), blocks.getLast()->str_len);
	}

	invokeOnChange();
	return true;
}

bool TBStyleEdit::getText(TBStr &text) {
	TBSelection tmp_selection(this);
	tmp_selection.selectAll();
	return tmp_selection.getText(text);
}

void TBStyleEdit::invokeOnChange() {
	listener->onChange();
	if (syntax_highlighter != nullptr) {
		syntax_highlighter->onChange(this);
	}
}

bool TBStyleEdit::isEmpty() const {
	return blocks.getFirst() == blocks.getLast() && blocks.getFirst()->str.isEmpty();
}

void TBStyleEdit::setAlign(TB_TEXT_ALIGN align) {
	this->align = align;
	// Call SetAlign on all blocks currently selected, or the block of the current caret position.
	TBBlock *start = selection.isSelected() ? selection.start.block : caret.pos.block;
	TBBlock *stop = selection.isSelected() ? selection.stop.block : caret.pos.block;
	while ((start != nullptr) && start != stop->getNext()) {
		start->setAlign(align);
		start = start->getNext();
	}
}

void TBStyleEdit::setMultiline(bool multiline) {
	packed.multiline_on = multiline;
}

void TBStyleEdit::setStyling(bool styling) {
	packed.styling_on = styling;
}

void TBStyleEdit::setReadOnly(bool readonly) {
	packed.read_only = readonly;
}

void TBStyleEdit::setSelection(bool selection) {
	packed.selection_on = selection;
}

void TBStyleEdit::setPassword(bool password) {
	if (packed.password_on == static_cast<int>(password)) {
		return;
	}
	packed.password_on = password;
	reformat(true);
}

void TBStyleEdit::setWrapping(bool wrapping) {
	if (packed.wrapping == static_cast<int>(wrapping)) {
		return;
	}
	packed.wrapping = wrapping;
	reformat(false);
}

// == TBUndoRedoStack ==================================================

TBUndoRedoStack::~TBUndoRedoStack() {
	clear(true, true);
}

void TBUndoRedoStack::undo(TBStyleEdit *styledit) {
	if (undos.getNumItems() == 0) {
		return;
	}
	TBUndoEvent *e = undos.remove(undos.getNumItems() - 1);
	redos.add(e);
	apply(styledit, e, true);
}

void TBUndoRedoStack::redo(TBStyleEdit *styledit) {
	if (redos.getNumItems() == 0) {
		return;
	}
	TBUndoEvent *e = redos.remove(redos.getNumItems() - 1);
	undos.add(e);
	apply(styledit, e, false);
}

void TBUndoRedoStack::apply(TBStyleEdit *styledit, TBUndoEvent *e, bool reverse) {
	applying = true;
	if (e->insert == reverse) {
		styledit->selection.selectNothing();
		styledit->caret.setGlobalOfs(e->gofs, false);
		core_assert(TBTextOfs(styledit->caret.pos).getGlobalOfs(styledit) == e->gofs);

		TBTextOfs start = styledit->caret.pos;
		styledit->caret.setGlobalOfs(e->gofs + e->text.length(), false);
		core_assert(TBTextOfs(styledit->caret.pos).getGlobalOfs(styledit) == e->gofs + e->text.length());

		styledit->selection.select(start, styledit->caret.pos);
		styledit->selection.removeContent();
	} else {
		styledit->selection.selectNothing();
		styledit->caret.setGlobalOfs(e->gofs, true, true);
		styledit->insertText(e->text);
		int text_len = e->text.length();
		if (text_len > 1) {
			styledit->selection.select(e->gofs, e->gofs + text_len);
		}
	}
	styledit->scrollIfNeeded(true, true);
	applying = false;
}

void TBUndoRedoStack::clear(bool clearUndo, bool clearRedo) {
	core_assert(!applying);
	if (clearUndo) {
		undos.deleteAll();
	}
	if (clearRedo) {
		redos.deleteAll();
	}
}

TBUndoEvent *TBUndoRedoStack::commit(TBStyleEdit *styledit, int32_t gofs, int32_t len, const char *text, bool insert) {
	if (applying || styledit->packed.read_only) {
		return nullptr;
	}
	clear(false, true);

	// If we're inserting a single character, check if we want to append it to the previous event.
	if (insert && (undos.getNumItems() != 0)) {
		int num_char = utf8::count_characters(text, len);
		TBUndoEvent *e = undos[undos.getNumItems() - 1];
		if (num_char == 1 && e->insert && e->gofs + e->text.length() == gofs) {
			// Appending a space to other space(s) should append
			if ((text[0] == ' ' && (strpbrk(e->text.c_str(), "\r\n") == nullptr)) ||
				// But non spaces should not
				(strpbrk(e->text.c_str(), " \r\n") == nullptr)) {
				e->text.append(text, len);
				return e;
			}
		}
	}

	// Create a new event
	if (TBUndoEvent *e = new TBUndoEvent()) {
		e->gofs = gofs;
		e->text.set(text, len);
		e->insert = insert;
		undos.add(e);
		return e;
	}

	// OOM
	clear(true, true);
	return nullptr;
}

} // namespace tb
