/**
 * @file
 */

#include "tb_menu_window.h"
#include "tb_widgets_listener.h"

namespace tb {

TBMenuWindow::TBMenuWindow(TBWidget *target, TBID id) : TBPopupWindow(target) {
	setID(id);
	setSkinBg(TBIDC("TBMenuWindow"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
	m_select_list.getScrollContainer()->setAdaptToContentSize(true);
	m_select_list.setIsFocusable(false); ///< Avoid it autoclosing its window on click
	m_select_list.setSkinBg("");
	m_select_list.setRect(getPaddingRect());
	m_select_list.setGravity(WIDGET_GRAVITY_ALL);
	addChild(&m_select_list);
}

TBMenuWindow::~TBMenuWindow() {
	removeChild(&m_select_list);
}

void TBMenuWindow::onDie() {
	m_select_list.setSource(nullptr);
}

bool TBMenuWindow::show(TBSelectItemSource *source, const TBPopupAlignment &alignment, int initialValue) {
	m_select_list.setValue(initialValue);
	m_select_list.setSource(source);
	m_select_list.validateList();

	return TBPopupWindow::show(alignment);
}

bool TBMenuWindow::onEvent(const TBWidgetEvent &ev) {
	if (ev.type == EVENT_TYPE_CLICK && &m_select_list == ev.target) {
		TBWidgetSafePointer this_widget(this);

		// Invoke the click on the target
		TBWidgetEvent target_ev(EVENT_TYPE_CLICK);
		target_ev.ref_id = ev.ref_id;
		invokeEvent(target_ev);

		// If target not deleted, close
		if (this_widget.get())
			close();
		return true;
	}
	return TBPopupWindow::onEvent(ev);
}

} // namespace tb
