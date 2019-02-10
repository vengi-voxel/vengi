/**
 * @file
 */

#include "tb_widgets_listener.h"
#include "tb_popup_window.h"

namespace tb {

TBRect TBPopupAlignment::getAlignedRect(TBWidget *popup, TBWidget *target) const
{
	TBWidget *root = target->getParentRoot();

	SizeConstraints sc(root->getRect().w, root->getRect().h);

	PreferredSize ps = popup->getPreferredSize(sc);

	// Amount of pixels that should be avoided if the target rect needs to be moved.
	int avoid_w = 0, avoid_h = 0;

	int x = 0, y = 0;
	int w = Min(ps.pref_w, root->getRect().w);
	int h = Min(ps.pref_h, root->getRect().h);

	if (pos_in_root.x != UNSPECIFIED &&
		pos_in_root.y != UNSPECIFIED)
	{
		// Position is specified in absolute root coords
		x = pos_in_root.x;
		y = pos_in_root.y;
		avoid_w = pos_offset.x;
		avoid_h = pos_offset.y;
		// Make sure it's moved into view horizontally
		if (align == TB_ALIGN_TOP || align == TB_ALIGN_BOTTOM)
			x = Clamp(x, 0, root->getRect().w - w);
	}
	else
	{
		target->convertToRoot(x, y);

		if (align == TB_ALIGN_TOP || align == TB_ALIGN_BOTTOM)
		{
			if (expand_to_target_width)
				w = Max(w, target->getRect().w);

			// If the menu is aligned top or bottom, limit its height to the worst case available height.
			// Being in the center of the root, that is half the root height minus the target rect.
			h = Min(h, root->getRect().h / 2 - target->getRect().h);
		}
		avoid_w = target->getRect().w;
		avoid_h = target->getRect().h;
	}

	if (align == TB_ALIGN_BOTTOM)
		y = y + avoid_h + h > root->getRect().h ? y - h : y + avoid_h;
	else if (align == TB_ALIGN_TOP)
		y = y - h < 0 ? y + avoid_h : y - h;
	else if (align == TB_ALIGN_RIGHT)
	{
		x = x + avoid_w + w > root->getRect().w ? x - w : x + avoid_w;
		y = Min(y, root->getRect().h - h);
	}
	else // if (align == TB_ALIGN_LEFT)
	{
		x = x - w < 0 ? x + avoid_w : x - w;
		y = Min(y, root->getRect().h - h);
	}
	return TBRect(x, y, w, h);
}

TBPopupWindow::TBPopupWindow(TBWidget *target)
	: m_target(target)
{
	TBWidgetListener::addGlobalListener(this);
	setSkinBg(TBIDC("TBPopupWindow"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
	setSettings(WINDOW_SETTINGS_NONE);
}

TBPopupWindow::~TBPopupWindow()
{
	TBWidgetListener::removeGlobalListener(this);
}

bool TBPopupWindow::show(const TBPopupAlignment &alignment)
{
	// Calculate and set a good size for the popup window
	setRect(alignment.getAlignedRect(this, m_target.get()));

	TBWidget *root = m_target.get()->getParentRoot();
	root->addChild(this);
	return true;
}

bool TBPopupWindow::onEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_KEY_DOWN && ev.special_key == TB_KEY_ESC)
	{
		close();
		return true;
	}
	return TBWindow::onEvent(ev);
}

void TBPopupWindow::onWidgetFocusChanged(TBWidget *widget, bool focused)
{
	if (focused && !isEventDestinationFor(widget))
		close();
}

bool TBPopupWindow::onWidgetInvokeEvent(TBWidget *widget, const TBWidgetEvent &ev)
{
	if ((ev.type == EVENT_TYPE_POINTER_DOWN || ev.type == EVENT_TYPE_CONTEXT_MENU) &&
		!isEventDestinationFor(ev.target))
		close();
	return false;
}

void TBPopupWindow::onWidgetDelete(TBWidget *widget)
{
	// If the target widget is deleted, close!
	if (!m_target.get())
		close();
}

bool TBPopupWindow::onWidgetDying(TBWidget *widget)
{
	// If the target widget or an ancestor of it is dying, close!
	if (widget == m_target.get() || widget->isAncestorOf(m_target.get()))
		close();
	return false;
}

} // namespace tb
