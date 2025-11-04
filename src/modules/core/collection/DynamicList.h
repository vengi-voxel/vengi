/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include <initializer_list>
#include <stddef.h>

namespace core {

/**
 * @brief Linked list
 *
 * @ingroup Collections
 */
template<class TYPE>
class DynamicList {
private:
	struct Node {
		alignas(TYPE) unsigned char valueStorage[sizeof(TYPE)]; // storage for TYPE
		Node *next = nullptr;

		// Accessor for value
		TYPE* valuePtr() { return reinterpret_cast<TYPE*>(valueStorage); }
		const TYPE* valuePtr() const { return reinterpret_cast<const TYPE*>(valueStorage); }

		// Construct value in-place
		template<typename ..._Args>
		inline void construct(_Args&&... args) {
			new (valueStorage) TYPE(core::forward<_Args>(args)...);
		}

		// Destroy value explicitly
		inline void destroy() {
			valuePtr()->~TYPE();
		}
	};

	Node *_first = nullptr;
	Node *_last = nullptr;
	// for node recycling
	Node *_freeList = nullptr;
	size_t _size = 0;

	template<typename... _Args>
	Node *acquireNode(_Args &&...args) {
		if (_freeList) {
			Node *node = _freeList;
			_freeList = _freeList->next;
			node->construct(core::forward<_Args>(args)...);
			node->next = nullptr;
			return node;
		}
		Node *node = new Node();
		node->construct(core::forward<_Args>(args)...);
		return node;
	}

	// Recycle node back into free list
	void recycleNode(Node *node) {
		node->destroy();
		node->next = _freeList;
		_freeList = node;
	}

public:
	using value_type = TYPE;

	DynamicList(std::initializer_list<TYPE> other) {
		for (auto &v : other) {
			emplace(v);
		}
	}
	DynamicList() = default;
	DynamicList(const DynamicList& other) {
		for (auto i = other.begin(); i != other.end(); ++i) {
			emplace(*i);
		}
	}
	DynamicList(DynamicList &&other) noexcept
		: _first(other._first), _last(other._last), _freeList(other._freeList), _size(other._size) {
		other._first = other._last = other._freeList = nullptr;
		other._size = 0;
	}

	~DynamicList() {
		release();
	}

	void release() {
		clear();
		Node *entry = _freeList;
		while (entry != nullptr) {
			Node *next = entry->next;
			delete entry;
			entry = next;
		}
		_freeList = nullptr;
	}

	DynamicList& operator=(const DynamicList& other) {
		if (&other == this) {
			return *this;
		}
		clear();
		for (auto i = other.begin(); i != other.end(); ++i) {
			emplace(*i);
		}
		return *this;
	}
	DynamicList &operator=(DynamicList &&other) {
		if (this != &other) {
			release();
			_first = other._first;
			_last = other._last;
			_freeList = other._freeList;
			_size = other._size;
			other._first = other._last = other._freeList = nullptr;
			other._size = 0;
		}
		return *this;
	}

	class iterator {
	friend class DynamicList;
	private:
		Node* _node;
		Node* _prev;

	public:
		constexpr iterator() :
			_node(nullptr), _prev(nullptr) {
		}
		iterator(Node* node, Node* prev) :
			_node(node), _prev(prev) {
		}

		CORE_FORCE_INLINE TYPE& operator*() {
			return *_node->valuePtr();
		}
		CORE_FORCE_INLINE const TYPE& operator*() const {
			return *_node->valuePtr();
		}

		iterator& operator++() {
			if (_node) {
				_prev = _node;
				_node = _node->next;
			}
			return *this;
		}

		CORE_FORCE_INLINE const Node* operator->() const {
			return _node;
		}

		CORE_FORCE_INLINE bool operator!=(const iterator& rhs) const {
			return _node != rhs._node;
		}

		CORE_FORCE_INLINE bool operator==(const iterator& rhs) const {
			return _node == rhs._node;
		}
	};

	inline bool empty() const {
		return _first == nullptr;
	}

	iterator begin() const {
		return iterator(_first, nullptr);
	}

	constexpr iterator end() const {
		return iterator();
	}

	inline size_t size() const {
		return _size;
	}

	void clear() {
		Node *entry = _first;
		while (entry != nullptr) {
			Node *next = entry->next;
			recycleNode(entry);
			entry = next;
		}
		_first = _last = nullptr;
		_size = 0;
	}

	template<typename... _Args>
	void emplace(_Args&&... args) {
		Node *node = acquireNode(core::forward<_Args>(args)...);
		if (!_last) {
			_first = _last = node;
		} else {
			_last->next = node;
			_last = node;
		}
		++_size;
	}

	bool insert(const TYPE& value) {
		emplace(value);
		return true;
	}

	bool insert(TYPE &&value) {
		emplace(core::move(value));
		return true;
	}

	bool insert_front(const TYPE& value) {
		Node *node = acquireNode(value);
		if (!_first) {
			_first = _last = node;
		} else {
			node->next = _first;
			_first = node;
		}
		++_size;
		return true;
	}

	const TYPE* back() const {
		return _last ? _last->valuePtr() : nullptr;
	}

	TYPE* back() {
		return _last ? _last->valuePtr() : nullptr;
	}

	/**
	 * @brief Only remove one element from the list. If there are more elements with the same value
	 * make sure to call this until false is returned.
	 */
	bool remove(const TYPE& value) {
		if (!_first) {
			return false;
		}

		if (*_first->valuePtr() == value) {
			Node *next = _first->next;
			recycleNode(_first);
			_first = next;
			if (!next) {
				_last = nullptr;
			}
			--_size;
			return true;
		}

		Node* prev = _first;
		Node* entry = _first->next;
		while (entry != nullptr) {
			if (*entry->valuePtr() == value) {
				prev->next = entry->next;
				if (entry == _last) {
					_last = prev;
				}
				recycleNode(entry);
				--_size;
				return true;
			}
			prev = entry;
			entry = entry->next;
		}
		return false;
	}

	iterator erase(iterator iter) {
		if (iter == end()) {
			return iter;
		}
		Node* node = iter._node;
		Node* next = node->next;
		Node* prev = iter._prev;
		if (node == _first) {
			_first = next;
		}
		if (node == _last) {
			_last = prev;
		}
		if (prev != nullptr) {
			prev->next = next;
		}

		recycleNode(node);
		--_size;
		return iterator(next, prev);
	}
};

} // namespace core
