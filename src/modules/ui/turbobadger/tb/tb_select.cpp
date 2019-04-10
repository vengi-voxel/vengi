/**
 * @file
 */

#include "tb_select.h"
#include "tb_language.h"
#include "tb_menu_window.h"
#include "tb_sort.h"
#include "tb_tempbuffer.h"
#include "tb_widgets_listener.h"

namespace tb {

// == Sort callback for sorting items ===================================================

int select_list_sort_cb(TBSelectItemSource *source, const int *a, const int *b) {
	int value = strcmp(source->getItemString(*a), source->getItemString(*b));
	return source->getSort() == TB_SORT_DESCENDING ? -value : value;
}

TBSelectList::TBSelectList()
	: m_value(-1), m_list_is_invalid(false), m_scroll_to_current(false),
	  m_header_lng_string_id(TBIDC("TBList.header")) {
	_sortCallback = select_list_sort_cb;
	setSource(&m_default_source);
	setIsFocusable(true);
	setSkinBg(TBIDC("TBSelectList"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
	m_container.setGravity(WIDGET_GRAVITY_ALL);
	m_container.setRect(getPaddingRect());
	addChild(&m_container);
	m_layout.setGravity(WIDGET_GRAVITY_ALL);
	m_layout.setAxis(AXIS_Y);
	m_layout.setSpacing(0);
	m_layout.setLayoutPosition(LAYOUT_POSITION_LEFT_TOP);
	m_layout.setLayoutDistributionPosition(LAYOUT_DISTRIBUTION_POSITION_LEFT_TOP);
	m_layout.setLayoutSize(LAYOUT_SIZE_AVAILABLE);
	m_container.getContentRoot()->addChild(&m_layout);
	m_container.setScrollMode(SCROLL_MODE_Y_AUTO);
	m_container.setAdaptContentSize(true);
}

TBSelectList::~TBSelectList() {
	m_layout.removeFromParent();
	m_container.removeFromParent();
	setSource(nullptr);
}

void TBSelectList::onSourceChanged() {
	invalidateList();
}

void TBSelectList::onItemChanged(int index) {
	if (m_list_is_invalid) { // We're updating all widgets soon.
		return;
	}

	TBWidget *old_widget = getItemWidget(index);
	if (old_widget == nullptr) { // We don't have this widget so we have nothing to update.
		return;
	}

	// Replace the old widget representing the item, with a new one. Preserve its state.
	WIDGET_STATE old_state = old_widget->getStateRaw();

	if (TBWidget *widget = createAndAddItemAfter(index, old_widget)) {
		widget->setStateRaw(old_state);
	}

	old_widget->removeFromParent();
	delete old_widget;
}

void TBSelectList::onItemAdded(int index) {
	if (m_list_is_invalid) { // We're updating all widgets soon.
		return;
	}

	// Sorting, filtering etc. makes it messy to handle dynamic addition of items.
	// Resort to invalidate the entire list (may even be faster anyway)
	invalidateList();
}

void TBSelectList::onItemRemoved(int index) {
	if (m_list_is_invalid) { // We're updating all widgets soon.
		return;
	}

	// Sorting, filtering etc. makes it messy to handle dynamic addition of items.
	// Resort to invalidate the entire list (may even be faster anyway)
	invalidateList();
}

void TBSelectList::onAllItemsRemoved() {
	invalidateList();
	m_value = -1;
}

void TBSelectList::setFilter(const char *filter) {
	TBStr new_filter;
	if ((filter != nullptr) && (*filter != 0)) {
		new_filter.set(filter);
	}
	if (m_filter.equals(new_filter)) {
		return;
	}
	m_filter.set(new_filter);
	invalidateList();
}

void TBSelectList::setHeaderString(const TBID &id) {
	if (m_header_lng_string_id == id) {
		return;
	}
	m_header_lng_string_id = id;
	invalidateList();
}

void TBSelectList::invalidateList() {
	if (m_list_is_invalid) {
		return;
	}
	m_list_is_invalid = true;
	invalidate();
}

void TBSelectList::validateList() {
	if (!m_list_is_invalid) {
		return;
	}
	m_list_is_invalid = false;
	// FIX: Could delete and create only the changed items (faster filter change)

	// Remove old items
	while (TBWidget *child = m_layout.getContentRoot()->getFirstChild()) {
		child->removeFromParent();
		delete child;
	}
	if ((m_source == nullptr) || (m_source->getNumItems() == 0)) {
		return;
	}

	// Create a sorted list of the items we should include using the current filter.
	TBTempBuffer sort_buf;
	if (!sort_buf.reserve(m_source->getNumItems() * sizeof(int))) {
		return; // Out of memory
	}
	int *sorted_index = (int *)sort_buf.getData();

	// Populate the sorted index list
	int num_sorted_items = 0;
	for (int i = 0; i < m_source->getNumItems(); i++) {
		if (m_filter.isEmpty() || m_source->filter(i, m_filter)) {
			sorted_index[num_sorted_items++] = i;
		}
	}

	// Sort
	if (m_source->getSort() != TB_SORT_NONE) {
		insertion_sort<TBSelectItemSource *, int>(sorted_index, num_sorted_items, m_source, _sortCallback);
	}

	// Show header if we only show a subset of all items.
	if (!m_filter.isEmpty()) {
		if (TBWidget *widget = new TBTextField()) {
			TBStr str;
			str.setFormatted(g_tb_lng->getString(m_header_lng_string_id), num_sorted_items, m_source->getNumItems());
			widget->setText(str);
			widget->setSkinBg(TBIDC("TBList.header"));
			widget->setState(WIDGET_STATE_DISABLED, true);
			widget->setGravity(WIDGET_GRAVITY_ALL);
			widget->data.setInt(-1);
			m_layout.getContentRoot()->addChild(widget);
		}
	}

	// Create new items
	for (int i = 0; i < num_sorted_items; i++) {
		createAndAddItemAfter(sorted_index[i], nullptr);
	}

	selectItem(m_value, true);

	// FIX: Should not scroll just because we update the list. Only automatically first time!
	m_scroll_to_current = true;
}

TBWidget *TBSelectList::createAndAddItemAfter(int index, TBWidget *reference) {
	if (TBWidget *widget = m_source->createItemWidget(index, this)) {
		// Use item data as widget to index lookup
		widget->data.setInt(index);
		m_layout.getContentRoot()->addChildRelative(widget, WIDGET_Z_REL_AFTER, reference);
		return widget;
	}
	return nullptr;
}

void TBSelectList::setValue(int value) {
	if (value == m_value) {
		return;
	}

	selectItem(m_value, false);
	m_value = value;
	selectItem(m_value, true);
	scrollToSelectedItem();

	TBWidgetEvent ev(EVENT_TYPE_CHANGED);
	if (TBWidget *widget = getItemWidget(m_value)) {
		ev.ref_id = widget->getID();
	}
	invokeEvent(ev);
}

TBID TBSelectList::getSelectedItemID() {
	if ((m_source != nullptr) && m_value >= 0 && m_value < m_source->getNumItems()) {
		return m_source->getItemID(m_value);
	}
	return TBID();
}

void TBSelectList::selectItem(int index, bool selected) {
	if (TBWidget *widget = getItemWidget(index)) {
		widget->setState(WIDGET_STATE_SELECTED, selected);
	}
}

TBWidget *TBSelectList::getItemWidget(int index) {
	if (index == -1) {
		return nullptr;
	}
	for (TBWidget *tmp = m_layout.getContentRoot()->getFirstChild(); tmp != nullptr; tmp = tmp->getNext()) {
		if (tmp->data.getInt() == index) {
			return tmp;
		}
	}
	return nullptr;
}

void TBSelectList::scrollToSelectedItem() {
	if (m_list_is_invalid) {
		m_scroll_to_current = true;
		return;
	}
	m_scroll_to_current = false;
	if (TBWidget *widget = getItemWidget(m_value)) {
		m_container.scrollIntoView(widget->getRect());
	} else {
		m_container.scrollTo(0, 0);
	}
}

void TBSelectList::onSkinChanged() {
	m_container.setRect(getPaddingRect());
}

void TBSelectList::onProcess() {
	validateList();
}

void TBSelectList::onProcessAfterChildren() {
	if (m_scroll_to_current) {
		scrollToSelectedItem();
	}
}

bool TBSelectList::onEvent(const TBWidgetEvent &ev) {
	if (ev.type == EVENT_TYPE_CLICK && ev.target->getParent() == m_layout.getContentRoot()) {
		// setValue (EVENT_TYPE_CHANGED) might cause something to delete this (f.ex closing
		// the dropdown menu. We want to sent another event, so ensure we're still around.
		TBWidgetSafePointer this_widget(this);

		int index = ev.target->data.getInt();
		setValue(index);

		// If we're still around, invoke the click event too.
		if (this_widget.get() != nullptr) {
			TBSelectList *target_list = this;
			// If the parent window is a TBMenuWindow, we will iterate up the event destination
			// chain to find the top TBMenuWindow and invoke the event there.
			// That way events in submenus will reach the caller properly, and seem like it was
			// invoked on the top menu.
			TBWindow *window = getParentWindow();
			while (TBMenuWindow *menu_win = TBSafeCast<TBMenuWindow>(window)) {
				target_list = menu_win->getList();
				window = menu_win->getEventDestination()->getParentWindow();
			}

			// Invoke the click event on the target list
			TBWidgetEvent ev(EVENT_TYPE_CLICK);
			if (TBWidget *widget = getItemWidget(m_value)) {
				ev.ref_id = widget->getID();
			}
			target_list->invokeEvent(ev);
		}
		return true;
	}
	if (ev.type == EVENT_TYPE_KEY_DOWN) {
		if (changeValue(ev.special_key)) {
			return true;
		}

		// Give the scroll container a chance to handle the key so it may
		// scroll. This matters if the list itself is focused instead of
		// some child view of any select item (since that would have passed
		// the container already)
		if (getScrollContainer()->onEvent(ev)) {
			return true;
		}
	}
	return false;
}

bool TBSelectList::changeValue(SPECIAL_KEY key) {
	if ((m_source == nullptr) || (m_layout.getContentRoot()->getFirstChild() == nullptr)) {
		return false;
	}

	bool forward;
	if (key == TB_KEY_HOME || key == TB_KEY_DOWN) {
		forward = true;
	} else if (key == TB_KEY_END || key == TB_KEY_UP) {
		forward = false;
	} else {
		return false;
	}

	TBWidget *item_root = m_layout.getContentRoot();
	TBWidget *current = getItemWidget(m_value);
	TBWidget *origin = nullptr;
	if (key == TB_KEY_HOME || ((current == nullptr) && key == TB_KEY_DOWN)) {
		current = item_root->getFirstChild();
	} else if (key == TB_KEY_END || ((current == nullptr) && key == TB_KEY_UP)) {
		current = item_root->getLastChild();
	} else {
		origin = current;
	}

	while (current != nullptr) {
		if (current != origin && !current->getDisabled()) {
			break;
		}
		current = forward ? current->getNext() : current->getPrev();
	}
	// Select and focus what we found
	if (current != nullptr) {
		setValue(current->data.getInt());
		return true;
	}
	return false;
}

// == TBSelectDropdown ==========================================

TBSelectDropdown::TBSelectDropdown() : m_value(-1) {
	setSource(&m_default_source);
	setSkinBg(TBIDC("TBSelectDropdown"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
	m_arrow.setSkinBg(TBIDC("TBSelectDropdown.arrow"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
	getContentRoot()->addChild(&m_arrow);
}

TBSelectDropdown::~TBSelectDropdown() {
	getContentRoot()->removeChild(&m_arrow);
	setSource(nullptr);
	closeWindow();
}

void TBSelectDropdown::onSourceChanged() {
	m_value = -1;
	if ((m_source != nullptr) && (m_source->getNumItems() != 0)) {
		setValue(0);
	}
}

void TBSelectDropdown::onItemChanged(int index) {
}

void TBSelectDropdown::setValue(int value) {
	if (value == m_value || (m_source == nullptr)) {
		return;
	}
	m_value = value;

	if (m_value < 0) {
		setText("");
	} else if (m_value < m_source->getNumItems()) {
		setText(m_source->getItemString(m_value));
	}

	TBWidgetEvent ev(EVENT_TYPE_CHANGED);
	invokeEvent(ev);
}

TBID TBSelectDropdown::getSelectedItemID() const {
	if ((m_source != nullptr) && m_value >= 0 && m_value < m_source->getNumItems()) {
		return m_source->getItemID(m_value);
	}
	return TBID();
}

void TBSelectDropdown::openWindow() {
	if ((m_source == nullptr) || (m_source->getNumItems() == 0) || (m_window_pointer.get() != nullptr)) {
		return;
	}

	if (TBMenuWindow *window = new TBMenuWindow(this, TBIDC("TBSelectDropdown.window"))) {
		m_window_pointer.set(window);
		window->setSkinBg(TBIDC("TBSelectDropdown.window"));
		window->show(m_source, TBPopupAlignment(), getValue());
	}
}

void TBSelectDropdown::closeWindow() {
	if (TBMenuWindow *window = getMenuIfOpen()) {
		window->close();
	}
}

TBMenuWindow *TBSelectDropdown::getMenuIfOpen() const {
	return TBSafeCast<TBMenuWindow>(m_window_pointer.get());
}

bool TBSelectDropdown::onEvent(const TBWidgetEvent &ev) {
	if (ev.target == this && ev.type == EVENT_TYPE_CLICK) {
		// Open the menu, or set the value and close it if already open (this will
		// happen when clicking by keyboard since that will call click on this button)
		if (TBMenuWindow *menu_window = getMenuIfOpen()) {
			TBWidgetSafePointer tmp(this);
			int value = menu_window->getList()->getValue();
			menu_window->die();
			if (tmp.get() != nullptr) {
				setValue(value);
			}
		} else {
			openWindow();
		}
		return true;
	}
	if (ev.target->getID() == TBIDC("TBSelectDropdown.window") && ev.type == EVENT_TYPE_CLICK) {
		// Set the value of the clicked item
		if (TBMenuWindow *menu_window = getMenuIfOpen()) {
			setValue(menu_window->getList()->getValue());
		}
		return true;
	}
	if (ev.target == this && (m_source != nullptr) && ev.isKeyEvent()) {
		if (TBMenuWindow *menu_window = getMenuIfOpen()) {
			// Redirect the key strokes to the list
			TBWidgetEvent redirected_ev(ev);
			return menu_window->getList()->invokeEvent(redirected_ev);
		}
	}
	return false;
}

} // namespace tb
