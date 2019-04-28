/**
 * @file
 */

#pragma once

#include "tb_linklist.h"
#include "tb_list.h"
#include "tb_value.h"

namespace tb {

class TBSelectItemSource;

enum TB_SORT {
	TB_SORT_NONE,	  ///< No sorting. Items appear in list order.
	TB_SORT_ASCENDING, ///< Ascending sort.
	TB_SORT_DESCENDING ///< Descending sort.
};

/** TBSelectItemViewer is the viewer for items provided by TBSelectItemSource.
	There can be multiple viewers for each source. The viewer will recieve
	callbacks when the source is changed, so it can update itself.
*/
class TBSelectItemViewer : public TBLinkOf<TBSelectItemViewer> {
public:
	TBSelectItemViewer() : m_source(nullptr) {
	}
	virtual ~TBSelectItemViewer() {
	}

	/** Set the source which should provide the items for this viewer.
		This source needs to live longer than this viewer.
		Set nullptr to unset currently set source. */
	void setSource(TBSelectItemSource *source);
	TBSelectItemSource *getSource() const {
		return m_source;
	}

	/** Called when the source has changed or been unset by calling SetSource. */
	virtual void onSourceChanged() = 0;

	/** Called when the item at the given index has changed in a way that should
		update the viewer. */
	virtual void onItemChanged(int index) = 0;

	/** Called when the item at the given index has been added. */
	virtual void onItemAdded(int index) = 0;

	/** Called when the item at the given index has been removed. */
	virtual void onItemRemoved(int index) = 0;

	/** Called when all items have been removed. */
	virtual void onAllItemsRemoved() = 0;

protected:
	TBSelectItemSource *m_source;
};

/** TBSelectItemSource is a item provider interface for list widgets (TBSelectList and
	TBSelectDropdown).

	Instead of feeding all list widgets with all items all the time, the list widgets
	will ask TBSelectItemSource when it needs it. The list widgets may also apply
	filtering so only a subset of all the items are shown.

	CreateItemWidget can be overridden to create any set of widget content for each item.

	This class has no storage of items. If you want an array storage of items,
	use the subclass TBSelectItemSourceList. If you implement your own storage,
	remember to call InvokeItem[Added/...] to notify viewers that they need to update.
*/
class TBSelectItemSource {
public:
	TBSelectItemSource() : m_sort(TB_SORT_NONE) {
	}
	virtual ~TBSelectItemSource();

	/** Return true if an item matches the given filter text.
		By default, it returns true if GetItemString contains filter. */
	virtual bool filter(int index, const char *filter);

	/** Get the string of a item. If a item has more than one string,
		return the one that should be used for inline-find (pressing keys
		in the list will scroll to the item starting with the same letters),
		and for sorting the list. */
	virtual const char *getItemString(int index) const = 0;

	/** Get the source to be used if this item should open a sub menu. */
	virtual TBSelectItemSource *getItemSubSource(int index) {
		return nullptr;
	}

	/** Get the skin image to be painted before the text for this item. */
	virtual TBID getItemImage(int /*index*/) const {
		return TBID();
	}

	/** Get the id of the item. */
	virtual TBID getItemID(int /*index*/) const {
		return TBID();
	}

	/** Create the item representation widget(s). By default, it will create
		a TBTextField for string-only items, and other types for items that
		also has image or submenu. */
	virtual TBWidget *createItemWidget(int index, TBSelectItemViewer *viewer);

	/** Get the number of items */
	virtual int getNumItems() const = 0;

	/** Set sort type. Default is TB_SORT_NONE. */
	void setSort(TB_SORT sort) {
		m_sort = sort;
	}
	TB_SORT getSort() const {
		return m_sort;
	}

	/** Invoke OnItemChanged on all open viewers for this source. */
	void invokeItemChanged(int index, TBSelectItemViewer *exclude_viewer = nullptr);
	void invokeItemAdded(int index);
	void invokeItemRemoved(int index);
	void invokeAllItemsRemoved();

private:
	friend class TBSelectItemViewer;
	TBLinkListOf<TBSelectItemViewer> m_viewers;
	TB_SORT m_sort;
};

/** TBSelectItemSourceList is a item provider for list widgets (TBSelectList and
	TBSelectDropdown). It stores items of the type specified by the template in an array. */
template <class T> class TBSelectItemSourceList : public TBSelectItemSource {
public:
	TBSelectItemSourceList() {
	}
	virtual ~TBSelectItemSourceList() {
		deleteAllItems();
	}
	virtual const char *getItemString(int index) const {
		return getItem(index)->str;
	}
	virtual TBSelectItemSource *getItemSubSource(int index) {
		return getItem(index)->sub_source;
	}
	virtual TBID getItemImage(int index) const {
		return getItem(index)->skin_image;
	}
	virtual TBID getItemID(int index) const {
		return getItem(index)->id;
	}
	virtual int getNumItems() const {
		return m_items.getNumItems();
	}
	virtual TBWidget *createItemWidget(int index, TBSelectItemViewer *viewer) {
		if (TBWidget *widget = TBSelectItemSource::createItemWidget(index, viewer)) {
			T *item = m_items[index];
			widget->setID(item->id);
			return widget;
		}
		return nullptr;
	}

	inline int getItemIndex(const T *item) const {
		return m_items.find(item);
	}

	inline bool isFirst(const T *item) const {
		const int idx = getItemIndex(item);
		return idx == 0;
	}

	inline bool isLast(const T *item) const {
		const int idx = getItemIndex(item);
		if (idx == -1) {
			return false;
		}
		return idx == getNumItems() - 1;
	}

	/** Add a new item at the given index. */
	bool addItem(T *item, int index) {
		if (m_items.add(item, index)) {
			invokeItemAdded(index);
			return true;
		}
		return false;
	}

	/** Add a new item last. */
	bool addItem(T *item) {
		return addItem(item, m_items.getNumItems());
	}

	/** Get the item at the given index. */
	T *getItem(int index) const {
		return m_items[index];
	}

	/** Delete the item at the given index. */
	void deleteItem(int index) {
		if (!m_items.getNumItems()) {
			return;
		}
		m_items.doDelete(index);
		invokeItemRemoved(index);
	}

	/** Delete all items. */
	void deleteAllItems() {
		if (!m_items.getNumItems()) {
			return;
		}
		m_items.deleteAll();
		invokeAllItemsRemoved();
	}

private:
	TBListOf<T> m_items;
};

/** TBGenericStringItem item for TBGenericStringItemSource.
	It has a string and may have a skin image and sub item source. */
class TBGenericStringItem {
public:
	TBGenericStringItem(const TBGenericStringItem &other)
		: str(other.str), id(other.id), sub_source(other.sub_source), tag(other.tag) {
	}
	TBGenericStringItem(const char *str) : str(str), sub_source(nullptr) {
	}
	TBGenericStringItem(const char *str, TBID id) : str(str), id(id), sub_source(nullptr) {
	}
	TBGenericStringItem(const char *str, TBSelectItemSource *subSource) : str(str), sub_source(subSource) {
	}
	const TBGenericStringItem &operator=(const TBGenericStringItem &other) {
		str.set(other.str);
		id = other.id;
		sub_source = other.sub_source;
		tag = other.tag;
		return *this;
	}

	void setSkinImage(const TBID &image) {
		skin_image = image;
	}

public:
	TBStr str;
	TBID id;
	TBID skin_image;
	TBSelectItemSource *sub_source;

	/** This value is free to use for anything. It's not used internally. */
	TBValue tag;
};

/** TBGenericStringItemSource is a item source list providing items of type TBGenericStringItem. */

class TBGenericStringItemSource : public TBSelectItemSourceList<TBGenericStringItem> {};

} // namespace tb
