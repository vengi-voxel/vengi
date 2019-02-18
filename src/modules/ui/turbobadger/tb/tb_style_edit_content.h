/**
 * @file
 */

#pragma once

#include "tb_core.h"

namespace tb {

class TBTextFragment;

/** Content for a non-text TBTextFragment. */
class TBTextFragmentContent {
public:
	virtual ~TBTextFragmentContent() {
	}

	/** Update the position of the content, relative to the first line of text (no scrolling applied). */
	virtual void updatePos(const TBBlock *block, int x, int y) {
	}

	virtual void paint(const TBPaintProps *props, TBTextFragment *fragment) {
	}
	virtual void click(const TBBlock *block, TBTextFragment *fragment, int button, uint32_t modifierkeys) {
	}
	virtual int32_t getWidth(const TBBlock *block, TBFontFace *font, TBTextFragment *fragment) {
		return 0;
	}
	virtual int32_t getHeight(const TBBlock *block, TBFontFace *font, TBTextFragment *fragment) {
		return 0;
	}
	virtual int32_t getBaseline(const TBBlock *block, TBFontFace *font, TBTextFragment *fragment) {
		return getHeight(block, font, fragment);
	}
	virtual bool getAllowBreakBefore(const TBBlock *block) {
		return true;
	}
	virtual bool getAllowBreakAfter(const TBBlock *block) {
		return true;
	}

	/** Get type of fragment content. All standard fragments return 0. */
	virtual uint32_t getType() {
		return 0;
	}
};

/** A horizontal line for TBStyleEdit. */
class TBTextFragmentContentHR : public TBTextFragmentContent {
public:
	TBTextFragmentContentHR(int32_t width_in_percent, int32_t height);

	virtual void paint(const TBPaintProps *props, TBTextFragment *fragment);
	virtual int32_t getWidth(const TBBlock *block, TBFontFace *font, TBTextFragment *fragment);
	virtual int32_t getHeight(const TBBlock *block, TBFontFace *font, TBTextFragment *fragment);

private:
	int32_t width_in_percent, height;
};

/** Fragment content that enables underline in a TBStyleEdit */
class TBTextFragmentContentUnderline : public TBTextFragmentContent {
public:
	TBTextFragmentContentUnderline() {
	}
	virtual void paint(const TBPaintProps *props, TBTextFragment *fragment);
	virtual bool getAllowBreakBefore(const TBBlock *block) {
		return true;
	}
	virtual bool getAllowBreakAfter(const TBBlock *block) {
		return false;
	}
};

/** Fragment content that changes color in a TBStyleEdit */
class TBTextFragmentContentTextColor : public TBTextFragmentContent {
public:
	TBColor color;
	TBTextFragmentContentTextColor(const TBColor &color) : color(color) {
	}
	virtual void paint(const TBPaintProps *props, TBTextFragment *fragment);
	virtual bool getAllowBreakBefore(const TBBlock *block) {
		return true;
	}
	virtual bool getAllowBreakAfter(const TBBlock *block) {
		return false;
	}
};

/** Fragment content that ends a change of style in a TBStyleEdit */
class TBTextFragmentContentStylePop : public TBTextFragmentContent {
public:
	virtual void paint(const TBPaintProps *props, TBTextFragment *fragment);
	virtual bool getAllowBreakBefore(const TBBlock *block) {
		return false;
	}
	virtual bool getAllowBreakAfter(const TBBlock *block) {
		return true;
	}
};

} // namespace tb
