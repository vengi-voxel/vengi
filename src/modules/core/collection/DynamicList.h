/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/Assert.h"
#include <initializer_list>

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
		template<typename ..._Args>
		inline Node(_Args&&... args) :
				value(core::forward<_Args>(args)...), next(nullptr) {
		}
		TYPE value;
		Node *next;
	};
	Node *_first = nullptr;
	Node *_last = nullptr;
public:
	using value_type = TYPE;

	DynamicList(std::initializer_list<TYPE> other) {
		for (auto i = other.begin(); i != other.end(); ++i) {
			insert(*i);
		}
	}
	DynamicList() {
	}
	DynamicList(const DynamicList& other) {
		for (auto i = other.begin(); i != other.end(); ++i) {
			insert(i->value);
		}
	}
	DynamicList(DynamicList &&other) noexcept
		: _first(other._first), _last(other._last) {
		other._first = other._last = nullptr;
	}

	~DynamicList() {
		clear();
	}

	DynamicList& operator=(const DynamicList& other) {
		if (&other == this) {
			return *this;
		}
		clear();
		for (auto i = other.begin(); i != other.end(); ++i) {
			insert(i->value);
		}
		return *this;
	}
	DynamicList &operator=(DynamicList &&other) noexcept {
		if (this != &other) {
			clear();
			_first = other._first;
			_last = other._last;
			other._first = other._last = nullptr;
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

		inline const TYPE& operator*() const {
			return _node->value;
		}

		inline TYPE& operator*() {
			return _node->value;
		}

		inline const TYPE& operator()() const {
			return _node->value;
		}

		inline TYPE& operator()() {
			return _node->value;
		}

		iterator& operator++() {
			if (_node->next != nullptr) {
				_prev = _node;
				_node = _node->next;
				return *this;
			}
			_prev = _node;
			_node = nullptr;
			return *this;
		}

		inline const Node* operator->() const {
			return _node;
		}

		inline bool operator!=(const iterator& rhs) const {
			return _node != rhs._node;
		}

		inline bool operator==(const iterator& rhs) const {
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

	size_t count() const {
		size_t cnt = 0u;
		Node *entry = _first;
		while (entry != nullptr) {
			++cnt;
			entry = entry->next;
		}
		return cnt;
	}

	void clear() {
		Node *entry = _first;
		while (entry != nullptr) {
			Node *prev = entry;
			entry = entry->next;
			delete prev;
		}
		_first = _last = nullptr;
	}

	template<typename... _Args>
	void emplace(_Args&&... args) {
		Node* node = new Node(core::forward<_Args>(args)...);
		if (node == nullptr) {
			return;
		}
		if (_last == nullptr) {
			core_assert(_first == nullptr);
			_first = _last = node;
		} else {
			_last->next = node;
			_last = node;
		}
	}

	bool insert(const TYPE& value) {
		Node* node = new Node(value);
		if (node == nullptr) {
			return false;
		}
		if (_last == nullptr) {
			core_assert(_first == nullptr);
			_first = _last = node;
		} else {
			_last->next = node;
			_last = node;
		}
		return true;
	}

	bool insert_front(const TYPE& value) {
		Node* node = new Node(value);
		if (node == nullptr) {
			return false;
		}
		if (_first == nullptr) {
			_first = _last = node;
		} else {
			node->next = _first;
			_first = node;
		}
		return true;
	}

	const TYPE* back() const {
		if (_last == nullptr) {
			return nullptr;
		}
		return &_last->value;
	}

	TYPE* back() {
		if (_last == nullptr) {
			return nullptr;
		}
		return &_last->value;
	}

	/**
	 * @brief Only remove one element from the list. If there are more elements with the same value
	 * make sure to call this until false is returned.
	 */
	bool remove(const TYPE& value) {
		if (_first == nullptr) {
			return false;
		}
		if (_first->value == value) {
			if (_first->next == nullptr) {
				delete _first;
				_first = _last = nullptr;
			} else {
				Node* next = _first->next;
				delete _first;
				_first = next;
			}
			return true;
		}

		Node* prev = _first;
		Node* entry = _first->next;

		while (entry != nullptr) {
			if (entry->value == value) {
				prev->next = entry->next;
				delete entry;
				return true;
			}
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
		delete node;
		return iterator(next, prev);
	}
};

}
