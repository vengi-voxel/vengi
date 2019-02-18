/**
 * @file
 */

#pragma once

#include "core/Assert.h"
#include "tb_core.h"

namespace tb {

class TBLinkList;
class TBLink;

/** TBLinkListIterator - The backend class for a safe iteration of a TBLinkList.

	You would normally recieve a typed iterator from a TBLinkListOf::iterateForward
	or TBLinkListOf::iterateBackward, instead of creating this object directly.

	Safe iteration means that if a link is removed from a linked list, _all_ iterators that currently
	point to that link will automatically step to the next link in the iterators direction. */

class TBLinkListIterator {
public:
	TBLinkListIterator(const TBLinkListIterator &iter);
	TBLinkListIterator(TBLinkList *linklist, TBLink *current_link, bool forward);
	~TBLinkListIterator();

	/** Set the iterator to the first link in we iterate forward,
		or set it to the last link if we iterate backward.  */
	void reset();

	/** Get the current link or nullptr if out of bounds. */
	TBLink *get() const {
		return m_current_link;
	}

	/** Get the current link and step the iterator to the next (forward or backward). */
	TBLink *getAndStep();

	operator TBLink *() const {
		return m_current_link;
	}

	const TBLinkListIterator &operator=(const TBLinkListIterator &iter);

private:
	TBLinkList *m_linklist; ///< The linklist we are iterating.
	TBLink *m_current_link; ///< The current link, or nullptr.
	bool m_forward;			///< true if we iterate from first to last item.

	TBLinkListIterator *m_prev; ///< Link in list of iterators for m_linklist
	TBLinkListIterator *m_next; ///< Link in list of iterators for m_linklist

	/** RemoveLink is called when removing/deleting links in the target linklist.
		This will make sure iterators skip the deleted item. */
	void removeLink(TBLink *link);
	friend class TBLinkList;

	/** Add ourself to the chain of iterators in the linklist. */
	void doRegister();

	/** Unlink ourself from the chain of iterators in the linklist. */
	void unregister();
	void unregisterAndClear();
};

/** TBLink - The backend class to be inserted in TBLinkList.
	Use the typed TBLinkOf for object storing! */

class TBLink {
public:
	TBLink() : prev(nullptr), next(nullptr), linklist(nullptr) {
	}

	/** Return true if the link is currently added to a list. */
	bool isInList() const {
		return linklist ? true : false;
	}

public:
	TBLink *prev;
	TBLink *next;
	TBLinkList *linklist;
};

template <class T> class TBLinkOf : public TBLink {
public:
	inline T *getPrev() const {
		return (T *)prev;
	}
	inline T *getNext() const {
		return (T *)next;
	}
};

/** TBLinkList - This is the backend for TBLinkListOf and TBLinkListAutoDeleteOf.
	You should use the typed TBLinkListOf or TBLinkListAutoDeleteOf for object storing! */

class TBLinkList {
public:
	TBLinkList() : first(nullptr), last(nullptr), first_iterator(nullptr) {
	}
	~TBLinkList();

	void remove(TBLink *link);
	void removeAll();

	void addFirst(TBLink *link);
	void addLast(TBLink *link);

	void addBefore(TBLink *link, TBLink *reference);
	void addAfter(TBLink *link, TBLink *reference);

	bool containsLink(TBLink *link) const {
		return link->linklist == this;
	}

	bool hasLinks() const {
		return first ? true : false;
	}

	int countLinks() const;

public:
	TBLink *first;
	TBLink *last;
	TBLinkListIterator *first_iterator;
};

/** TBLinkListOf is a double linked linklist. */

template <class T> class TBLinkListOf {
public:
	/** Remove link from this linklist. */
	void remove(T *link) {
		m_linklist.remove(static_cast<TBLinkOf<T> *>(link));
	}

	/** Remove link from this linklist and delete it. */
	void doDelete(T *link) {
		m_linklist.remove(static_cast<TBLinkOf<T> *>(link));
		delete link;
	}

	/** Remove all links without deleting them. */
	void removeAll() {
		m_linklist.removeAll();
	}

	/** Delete all links in this linklist. */
	void deleteAll() {
		while (T *t = getFirst())
			doDelete(t);
	}

	/** Add link first in this linklist. */
	void addFirst(T *link) {
		m_linklist.addFirst(static_cast<TBLinkOf<T> *>(link));
	}

	/** Add link last in this linklist. */
	void addLast(T *link) {
		m_linklist.addLast(static_cast<TBLinkOf<T> *>(link));
	}

	/** Add link before the reference link (which must be added to this linklist). */
	void addBefore(T *link, T *reference) {
		m_linklist.addBefore(static_cast<TBLinkOf<T> *>(link), reference);
	}

	/** Add link after the reference link (which must be added to this linklist). */
	void addAfter(T *link, T *reference) {
		m_linklist.addAfter(static_cast<TBLinkOf<T> *>(link), reference);
	}

	/** Return true if the link is currently added to this linklist. */
	bool containsLink(T *link) const {
		return m_linklist.containsLink(static_cast<TBLinkOf<T> *>(link));
	}

	/** Get the first link, or nullptr. */
	T *getFirst() const {
		return (T *)static_cast<TBLinkOf<T> *>(m_linklist.first);
	}

	/** Get the last link, or nullptr. */
	T *getLast() const {
		return (T *)static_cast<TBLinkOf<T> *>(m_linklist.last);
	}

	/** Return true if this linklist contains any links. */
	bool hasLinks() const {
		return m_linklist.hasLinks();
	}

	/** Count the number of links in this list by iterating through all links. */
	int countLinks() const {
		return m_linklist.countLinks();
	}

	/** Typed iterator for safe iteration. For more info, see TBLinkListIterator. */
	class Iterator : public TBLinkListIterator {
	public:
		Iterator(TBLinkListOf<T> *linklistof, bool forward)
			: TBLinkListIterator(&linklistof->m_linklist,
								 forward ? linklistof->m_linklist.first : linklistof->m_linklist.last, forward) {
		}
		Iterator(TBLinkListOf<T> *linklistof, T *link, bool forward)
			: TBLinkListIterator(&linklistof->m_linklist, link, forward) {
		}
		inline T *get() const {
			return (T *)static_cast<TBLinkOf<T> *>(TBLinkListIterator::get());
		}
		inline T *getAndStep() {
			return (T *)static_cast<TBLinkOf<T> *>(TBLinkListIterator::getAndStep());
		}
		inline operator T *() const {
			return (T *)static_cast<TBLinkOf<T> *>(get());
		}
	};

	/** Get a forward iterator that starts with the first link. */
	Iterator iterateForward() {
		return Iterator(this, true);
	}

	/** Get a forward iterator that starts with the given link. */
	Iterator iterateForward(T *link) {
		return Iterator(this, link, true);
	}

	/** Get a backward iterator that starts with the last link. */
	Iterator iterateBackward() {
		return Iterator(this, false);
	}

	/** Get a backward iterator that starts with the given link. */
	Iterator iterateBackward(T *link) {
		return Iterator(this, link, false);
	}

private:
	TBLinkList m_linklist;
};

/** TBLinkListAutoDeleteOf is a double linked linklist that deletes all links on destruction. */

template <class T> class TBLinkListAutoDeleteOf : public TBLinkListOf<T> {
public:
	~TBLinkListAutoDeleteOf() {
		TBLinkListOf<T>::deleteAll();
	}
};

} // namespace tb
