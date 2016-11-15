// ================================================================================
// ==      This file is a part of Turbo Badger. (C) 2011-2014, Emil Segerås      ==
// ==                     See tb_core.h for more information.                    ==
// ================================================================================

#ifndef TB_STYLE_EDIT_CONTENT_H
#define TB_STYLE_EDIT_CONTENT_H

#include "tb_core.h"

namespace tb {

class TBTextFragment;

/** Content for a non-text TBTextFragment. */

class TBTextFragmentContent
{
public:
	virtual ~TBTextFragmentContent() {}

	/** Update the position of the content, relative to the first line of text (no scrolling applied). */
	virtual void UpdatePos(const TBBlock *block, int x, int y) {}

	virtual void Paint(const TBPaintProps *props, TBTextFragment *fragment) {}
	virtual void Click(const TBBlock *block, TBTextFragment *fragment, int button, uint32 modifierkeys) {}
	virtual int32 GetWidth(const TBBlock *block, TBFontFace *font, TBTextFragment *fragment) { return 0; }
	virtual int32 GetHeight(const TBBlock *block, TBFontFace *font, TBTextFragment *fragment) { return 0; }
	virtual int32 GetBaseline(const TBBlock *block, TBFontFace *font, TBTextFragment *fragment) { return GetHeight(block, font, fragment); }
	virtual bool GetAllowBreakBefore(const TBBlock *block) { return true; }
	virtual bool GetAllowBreakAfter(const TBBlock *block) { return true; }

	/** Get type of fragment content. All standard fragments return 0. */
	virtual uint32 GetType()		{ return 0; }
};

/** A horizontal line for TBStyleEdit. */

class TBTextFragmentContentHR : public TBTextFragmentContent
{
public:
	TBTextFragmentContentHR(int32 width_in_percent, int32 height);

	virtual void Paint(const TBPaintProps *props, TBTextFragment *fragment);
	virtual int32 GetWidth(const TBBlock *block, TBFontFace *font, TBTextFragment *fragment);
	virtual int32 GetHeight(const TBBlock *block, TBFontFace *font, TBTextFragment *fragment);
private:
	int32 width_in_percent, height;
};

/** Fragment content that enables underline in a TBStyleEdit */

class TBTextFragmentContentUnderline : public TBTextFragmentContent
{
public:
	TBTextFragmentContentUnderline() {}
	virtual void Paint(const TBPaintProps *props, TBTextFragment *fragment);
	virtual bool GetAllowBreakBefore(const TBBlock *block) { return true; }
	virtual bool GetAllowBreakAfter(const TBBlock *block) { return false; }
};

/** Fragment content that changes color in a TBStyleEdit */

class TBTextFragmentContentTextColor : public TBTextFragmentContent
{
public:
	TBColor color;
	TBTextFragmentContentTextColor(const TBColor &color) : color(color) {}
	virtual void Paint(const TBPaintProps *props, TBTextFragment *fragment);
	virtual bool GetAllowBreakBefore(const TBBlock *block) { return true; }
	virtual bool GetAllowBreakAfter(const TBBlock *block) { return false; }
};

/** Fragment content that ends a change of style in a TBStyleEdit */

class TBTextFragmentContentStylePop : public TBTextFragmentContent
{
public:
	virtual void Paint(const TBPaintProps *props, TBTextFragment *fragment);
	virtual bool GetAllowBreakBefore(const TBBlock *block) { return false; }
	virtual bool GetAllowBreakAfter(const TBBlock *block) { return true; }
};

} // namespace tb

#endif // TB_STYLE_EDIT_CONTENT_H
