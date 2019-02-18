/**
 * @file
 */

#include "tb_widgets_listener.h"

namespace tb {

TBLinkListOf<TBWidgetListenerGlobalLink> g_listeners;

void TBWidgetListener::addGlobalListener(TBWidgetListener *listener) {
	g_listeners.addLast(listener);
}

void TBWidgetListener::removeGlobalListener(TBWidgetListener *listener) {
	g_listeners.remove(listener);
}

void TBWidgetListener::invokeWidgetDelete(TBWidget *widget) {
	TBLinkListOf<TBWidgetListenerGlobalLink>::Iterator global_i = g_listeners.iterateForward();
	TBLinkListOf<TBWidgetListener>::Iterator local_i = widget->m_listeners.iterateForward();
	while (TBWidgetListener *listener = local_i.getAndStep()) {
		listener->onWidgetDelete(widget);
	}
	while (TBWidgetListenerGlobalLink *link = global_i.getAndStep()) {
		static_cast<TBWidgetListener *>(link)->onWidgetDelete(widget);
	}
}

bool TBWidgetListener::invokeWidgetDying(TBWidget *widget) {
	bool handled = false;
	TBLinkListOf<TBWidgetListenerGlobalLink>::Iterator global_i = g_listeners.iterateForward();
	TBLinkListOf<TBWidgetListener>::Iterator local_i = widget->m_listeners.iterateForward();
	while (TBWidgetListener *listener = local_i.getAndStep()) {
		handled |= listener->onWidgetDying(widget);
	}
	while (TBWidgetListenerGlobalLink *link = global_i.getAndStep()) {
		handled |= static_cast<TBWidgetListener *>(link)->onWidgetDying(widget);
	}
	return handled;
}

void TBWidgetListener::invokeWidgetAdded(TBWidget *parent, TBWidget *child) {
	TBLinkListOf<TBWidgetListenerGlobalLink>::Iterator global_i = g_listeners.iterateForward();
	TBLinkListOf<TBWidgetListener>::Iterator local_i = parent->m_listeners.iterateForward();
	while (TBWidgetListener *listener = local_i.getAndStep()) {
		listener->onWidgetAdded(parent, child);
	}
	while (TBWidgetListenerGlobalLink *link = global_i.getAndStep()) {
		static_cast<TBWidgetListener *>(link)->onWidgetAdded(parent, child);
	}
}

void TBWidgetListener::invokeWidgetRemove(TBWidget *parent, TBWidget *child) {
	TBLinkListOf<TBWidgetListenerGlobalLink>::Iterator global_i = g_listeners.iterateForward();
	TBLinkListOf<TBWidgetListener>::Iterator local_i = parent->m_listeners.iterateForward();
	while (TBWidgetListener *listener = local_i.getAndStep()) {
		listener->onWidgetRemove(parent, child);
	}
	while (TBWidgetListenerGlobalLink *link = global_i.getAndStep()) {
		static_cast<TBWidgetListener *>(link)->onWidgetRemove(parent, child);
	}
}

void TBWidgetListener::invokeWidgetFocusChanged(TBWidget *widget, bool focused) {
	TBLinkListOf<TBWidgetListenerGlobalLink>::Iterator global_i = g_listeners.iterateForward();
	TBLinkListOf<TBWidgetListener>::Iterator local_i = widget->m_listeners.iterateForward();
	while (TBWidgetListener *listener = local_i.getAndStep()) {
		listener->onWidgetFocusChanged(widget, focused);
	}
	while (TBWidgetListenerGlobalLink *link = global_i.getAndStep()) {
		static_cast<TBWidgetListener *>(link)->onWidgetFocusChanged(widget, focused);
	}
}

bool TBWidgetListener::invokeWidgetInvokeEvent(TBWidget *widget, const TBWidgetEvent &ev) {
	bool handled = false;
	TBLinkListOf<TBWidgetListenerGlobalLink>::Iterator global_i = g_listeners.iterateForward();
	TBLinkListOf<TBWidgetListener>::Iterator local_i = widget->m_listeners.iterateForward();
	while (TBWidgetListener *listener = local_i.getAndStep()) {
		handled |= listener->onWidgetInvokeEvent(widget, ev);
	}
	while (TBWidgetListenerGlobalLink *link = global_i.getAndStep()) {
		handled |= static_cast<TBWidgetListener *>(link)->onWidgetInvokeEvent(widget, ev);
	}
	return handled;
}

void TBWidgetSafePointer::set(TBWidget *widget) {
	if (m_widget == widget) {
		return;
	}
	if (m_widget != nullptr) {
		m_widget->removeListener(this);
	}
	m_widget = widget;
	if (m_widget != nullptr) {
		m_widget->addListener(this);
	}
}

void TBWidgetSafePointer::onWidgetDelete(TBWidget *widget) {
	if (widget == m_widget) {
		set(nullptr);
	}
}

} // namespace tb
