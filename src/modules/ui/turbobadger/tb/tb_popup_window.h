/**
 * @file
 */

#pragma once

#include "tb_widgets_listener.h"
#include "tb_window.h"

namespace tb {

/** TBPopupAlignment describes the preferred alignment of a popup
	relative to a target widget or a given point.

	It calculates the rect to be used to match these preferences
	for any given popup and target. */
class TBPopupAlignment {
public:
	static const int UNSPECIFIED = TB_INVALID_DIMENSION;

	/** Align relative to the target widget. */
	TBPopupAlignment(TB_ALIGN align = TB_ALIGN_BOTTOM)
		: pos_in_root(UNSPECIFIED, UNSPECIFIED), align(align), expand_to_target_width(true) {
	}

	/** Align relative to the given position (coordinates relative to the root widget). */
	TBPopupAlignment(const TBPoint &posInRoot, TB_ALIGN align = TB_ALIGN_BOTTOM)
		: pos_in_root(posInRoot), align(align), expand_to_target_width(true) {
	}

	/** Align relative to the given position (coordinates relative to the root widget).
		Applies an additional offset. */
	TBPopupAlignment(const TBPoint &posInRoot, const TBPoint &posOffset)
		: pos_in_root(posInRoot), pos_offset(posOffset), align(TB_ALIGN_BOTTOM), expand_to_target_width(true) {
	}

	/** Calculate a good rect for the given popup window using its preferred size and
		the preferred alignment information stored in this class. */
	TBRect getAlignedRect(TBWidget *popup, TBWidget *target) const;

	TBPoint pos_in_root;
	TBPoint pos_offset;

	TB_ALIGN align;
	/** If true, the width of the popup will be at least the same as the target widget
		if the alignment is TB_ALIGN_TOP or TB_ALIGN_BOTTOM. */
	bool expand_to_target_width;
};

/** TBPopupWindow is a popup window that redirects any child widgets events
	through the given target. It will automatically close on click events that
	are not sent through this popup. */

class TBPopupWindow : public TBWindow, private TBWidgetListener {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBPopupWindow, TBWindow);

	TBPopupWindow(TBWidget *target);
	~TBPopupWindow();

	bool show(const TBPopupAlignment &alignment);

	virtual TBWidget *getEventDestination() override {
		return m_target.get();
	}

	virtual bool onEvent(const TBWidgetEvent &ev) override;

private:
	TBWidgetSafePointer m_target;
	// TBWidgetListener
	virtual void onWidgetFocusChanged(TBWidget *widget, bool focused) override;
	virtual bool onWidgetInvokeEvent(TBWidget *widget, const TBWidgetEvent &ev) override;
	virtual void onWidgetDelete(TBWidget *widget) override;
	virtual bool onWidgetDying(TBWidget *widget) override;
};

} // namespace tb
