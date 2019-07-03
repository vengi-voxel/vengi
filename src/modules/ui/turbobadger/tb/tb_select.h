/**
 * @file
 */

#pragma once

#include "tb_scroll_container.h"
#include "tb_select_item.h"
#include "tb_window.h"

namespace tb {

class TBMenuWindow;

/** TBSelectList shows a scrollable list of items provided by a TBSelectItemSource. */

class TBSelectList : public TBWidget, public TBSelectItemViewer {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBSelectList, TBWidget);

	TBSelectList();
	~TBSelectList();

	/** Get the default item source for this widget. This source can be used to add
		items of type TBGenericStringItem to this widget.
		It is the item source that is fed from resource files.

		If you need to add other types of items, or if you want to share item sources
		between several TBSelectDropDown/TBSelectList widgets, use SetSource using a
		external item source. */
	TBGenericStringItemSource *getDefaultSource() {
		return &m_default_source;
	}

	/** Set filter string so only matching items will be showed.
		Set nullptr or empty string to remove filter and show all items. */
	void setFilter(const char *filter);
	const char *getFilter() const {
		return m_filter;
	}

	/** Set the language string id for the header. The header is shown
		at the top of the list when only a subset of all items are shown. */
	void setHeaderString(const TBID &id);

	/** Make the list update its items to reflect the items from the
		in the current source. The update will take place next time
		the list is validated. */
	void invalidateList();

	/** Make sure the list is reflecting the current items in the source. */
	void validateList();

	/** The value is the selected item. In lists with multiple selectable
		items it's the item that is the current focus. */
	virtual void setValue(int value) override;
	virtual int getValue() const override {
		return m_value;
	}

	/** Get the ID of the selected item, or 0 if there is no item selected. */
	TBID getSelectedItemID() const;

	/** Change the value to a non disabled item that is visible with the current
		filter. Returns true if it successfully found another item.
		Valid keys:
			TB_KEY_UP - Previous item.
			TB_KEY_DOWN - Next item.
			TB_KEY_HOME - First item.
			TB_KEY_END - Last item. */
	bool changeValue(SPECIAL_KEY key);

	/** Set the selected state of the item at the given index. If you want
		to unselect the previously selected item, use setValue. */
	void selectItem(int index, bool selected);
	TBWidget *getItemWidget(int index);

	/** Scroll to the current selected item. The scroll may be delayed until
		the items has been layouted if the layout is currently invalid. */
	void scrollToSelectedItem();

	/** Return the scrollcontainer used in this list. */
	TBScrollContainer *getScrollContainer() {
		return &m_container;
	}

	virtual void onInflate(const INFLATE_INFO &info) override;
	virtual void onSkinChanged() override;
	virtual void onProcess() override;
	virtual void onProcessAfterChildren() override;
	virtual bool onEvent(const TBWidgetEvent &ev) override;

	// == TBSelectItemViewer ==================================================
	virtual void onSourceChanged() override;
	virtual void onItemChanged(int index) override;
	virtual void onItemAdded(int index) override;
	virtual void onItemRemoved(int index) override;
	virtual void onAllItemsRemoved() override;

	inline void setSortCallback(int (*func)(TBSelectItemSource *source, const int *a, const int *b)) {
		_sortCallback = func;
	}

protected:
	TBScrollContainer m_container;
	TBLayout m_layout;
	TBGenericStringItemSource m_default_source;
	int m_value;
	TBStr m_filter;
	bool m_list_is_invalid;
	bool m_scroll_to_current;
	TBID m_header_lng_string_id;

	int (*_sortCallback)(TBSelectItemSource *source, const int *a, const int *b);

private:
	TBWidget *createAndAddItemAfter(int index, TBWidget *reference);
};

/** TBSelectDropdown shows a button that opens a popup with a TBSelectList with items
	provided by a TBSelectItemSource. */

class TBSelectDropdown : public TBButton, public TBSelectItemViewer {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBSelectDropdown, TBButton);

	TBSelectDropdown();
	~TBSelectDropdown();

	/** Get the default item source for this widget. This source can be used to add
		items of type TBGenericStringItem to this widget.
		It is the item source that is fed from resource files.

		If you need to add other types of items, or if you want to share item sources
		between several TBSelectDropDown/TBSelectList widgets, use SetSource using a
		external item source. */
	TBGenericStringItemSource *getDefaultSource() {
		return &m_default_source;
	}

	/** Set the selected item. */
	virtual void setValue(int value) override;
	virtual int getValue() const override {
		return m_value;
	}

	/** Get the ID of the selected item, or 0 if there is no item selected. */
	TBID getSelectedItemID() const;

	/** Open the window if the model has items. */
	void openWindow();

	/** Close the window if it is open. */
	void closeWindow();

	/** Return the menu window if it's open, or nullptr. */
	TBMenuWindow *getMenuIfOpen() const;

	virtual void onInflate(const INFLATE_INFO &info) override;
	virtual bool onEvent(const TBWidgetEvent &ev) override;

	// == TBSelectItemViewer ==================================================
	virtual void onSourceChanged() override;
	virtual void onItemChanged(int index) override;
	virtual void onItemAdded(int index) override {
	}
	virtual void onItemRemoved(int index) override {
	}
	virtual void onAllItemsRemoved() override {
	}

protected:
	TBGenericStringItemSource m_default_source;
	TBSkinImage m_arrow;
	int m_value;
	TBWidgetSafePointer m_window_pointer; ///< Points to the dropdown window if opened
};

} // namespace tb
