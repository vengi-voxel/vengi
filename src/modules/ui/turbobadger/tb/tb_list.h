/**
 * @file
 */

#pragma once

#include "tb_core.h"
namespace tb {

/** TBList is a list (array) of pointers to any kind of objects.
	This is the backend for TBListOf and TBListAutoDeleteOf.
	You should use the typed TBListOf or TBListAutoDeleteOf for object storing! */
class TBListBackend {
public:
	TBListBackend() : m_data(nullptr) {
	}
	~TBListBackend() {
		removeAll();
	}
	bool reserve(int newCapacity);
	bool growIfNeeded();
	bool add(void *data);
	bool add(void *data, int index);
	void set(void *data, int index);
	void *get(int index) const;
	void *operator[](int index) const {
		return get(index);
	}
	void *removeFast(int index);
	void *remove(int index);
	void removeAll();
	void swap(int index1, int index2);
	int find(void *data) const;
	int getNumItems() const {
		return m_data != nullptr ? m_data->num : 0;
	}
	int getCapacity() const {
		return m_data != nullptr ? m_data->capacity : 0;
	}

private:
	struct TBLIST_DATA {
		int num;
		int capacity;
		void **list;
	};
	TBLIST_DATA *m_data;
};

/** TBListOf is a list (array) of pointers to the specified object type.
	Note: The objects won't be deleted automatically. If you want that,
	use TBListAutoDeleteOf! */
template <class T> class TBListOf {
public:
	/** Make sure there is space for at least num items in the list. Returns false on OOM failure. */
	bool reserve(int num) {
		return m_list.reserve(num);
	}

	/** Make sure there is space for at least one more item in the list. Returns false on OOM failure.
		There's no need to call this, but it can make OOM handling easier in some situations since you
		can guarantee there is space is in a list *before* you allocate an object to insert into it. */
	bool growIfNeeded() {
		return m_list.growIfNeeded();
	}

	/** Add data at the end of the list. Returns false on OOM failure. */
	bool add(T *data) {
		return m_list.add(data);
	}

	/** Add data at the given index in the list. Returns false on OOM failure. */
	bool add(T *data, int index) {
		return m_list.add(data, index);
	}

	/** Replace the item at the index with the new data */
	void set(T *data, int index) {
		m_list.set(data, index);
	}

	/** Returns the content at position index. */
	T *get(int index) const {
		return (T *)m_list.get(index);
	}

	/** Returns the content at position index. */
	T *operator[](int index) const {
		return (T *)m_list.get(index);
	}

	/** Remove the item at position index from the list and returns the pointer.
		This method should only be used when the order of the list is not important.
		If the order is important, use Remove() */
	T *removeFast(int index) {
		return (T *)m_list.removeFast(index);
	}

	/** Remove the item at position index from the list and returns the pointer. */
	T *remove(int index) {
		return (T *)m_list.remove(index);
	}

	/** Deletes the item at position index after removing it from the list.
		This method should only be used when the order of the list is not important.
		If the order is important, use Delete() */
	void deleteFast(int index) {
		delete (T *)m_list.removeFast(index);
	}

	/** Deletes the item at position index after removing it from the list. */
	void doDelete(int index) {
		delete (T *)m_list.remove(index);
	}

	/** Remove all items without deleding them. */
	void removeAll() {
		m_list.removeAll();
	}

	/** Remove and delete all items from the list. */
	void deleteAll() {
		for (int i = 0; i < getNumItems(); i++) {
			delete (T *)get(i);
		}
		m_list.removeAll();
	}

	/** Swap the items at index1 and index2 */
	void swap(int index1, int index2) {
		m_list.swap(index1, index2);
	}

	/** Search for the item with the given data and return the found index, or -1 if not found. */
	int find(T *data) const {
		return m_list.find(data);
	}

	/** Get the number of items in the list. */
	int getNumItems() const {
		return m_list.getNumItems();
	}

	/** Get the capacity of the list number of items it can hold without allocating more memory) */
	int getCapacity() const {
		return m_list.getCapacity();
	}

private:
	TBListBackend m_list;
};

/** TBListAutoDeleteOf is a list (array) of pointers to the specified object type.
	The objects will be deleted automatically on destruction. */
template <class T> class TBListAutoDeleteOf : public TBListOf<T> {
public:
	~TBListAutoDeleteOf() {
		TBListOf<T>::deleteAll();
	}
};

} // namespace tb
