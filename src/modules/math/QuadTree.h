/**
 * @file
 */

#pragma once

#include <vector>
#include <list>
#include <array>
#include <algorithm>
#include <unordered_map>
#include "core/GLM.h"
#include "Rect.h"
#include "core/Trace.h"

namespace math {

template<class NODE, typename TYPE>
class QuadTree {
public:
	typedef std::list<NODE> Contents;
private:
	class QuadTreeNode {
		friend class QuadTree;
	private:
		static inline Rect<TYPE> rect(const typename std::remove_pointer<NODE>::type* item) {
			return item->getRect();
		}

		static inline Rect<TYPE> rect(const typename std::remove_pointer<NODE>::type& item) {
			return item.getRect();
		}

		const int _maxDepth;
		int _depth;
		const Rect<TYPE> _area;
		Contents _contents;
		std::vector<QuadTreeNode> _nodes;

		QuadTreeNode(const Rect<TYPE>& bounds, int maxDepth, int depth) :
				_maxDepth(maxDepth), _depth(depth), _area(bounds) {
		}

	public:
		QuadTreeNode(QuadTreeNode&& node) :
				_maxDepth(node._maxDepth), _depth(node._depth), _area(node._area),
				_contents(std::move(node._contents)), _nodes(std::move(node._nodes)) {
		}

		QuadTreeNode(const QuadTreeNode& node) :
				_maxDepth(node._maxDepth), _depth(node._depth), _area(node._area),
				_contents(node._contents), _nodes(node._nodes) {
		}

	private:
		void createNodes() {
			if (_depth >= _maxDepth) {
				return;
			}
			if (glm::all(glm::lessThanEqual(getRect().size(), glm::one<glm::tvec2<TYPE> >()))) {
				return;
			}

			std::array<Rect<TYPE>, 4> subareas;
			getRect().split(subareas);
			_nodes.reserve(subareas.size());
			for (size_t i = 0; i < subareas.size(); ++i) {
				_nodes.emplace_back(QuadTreeNode(subareas[i], _maxDepth, _depth + 1));
			}
		}

		inline int count() const {
			int count = 0;
			for (const QuadTreeNode& node : _nodes) {
				count += node.count();
			}
			count += (int)_contents.size();
			return count;
		}

		inline const Rect<TYPE>& getRect() const {
			return _area;
		}

		inline const Contents& getContents() const {
			return _contents;
		}

		void getAllContents(Contents& results) const {
			for (const QuadTreeNode& node : _nodes) {
				if (node.isEmpty()) {
					continue;
				}
				node.getAllContents(results);
			}

			// faster for list - but slower for vector
			std::copy(_contents.begin(), _contents.end(), std::back_inserter(results));
		}

		bool remove(const NODE& item) {
			const Rect<TYPE>& area = rect(item);
			if (!getRect().contains(area)) {
				return false;
			}
			for (QuadTreeNode& node : _nodes) {
				if (!node.remove(item)) {
					continue;
				}
				return true;
			}

			auto i = std::find(_contents.begin(), _contents.end(), item);
			if (i == _contents.end()) {
				return false;
			}
			_contents.erase(i);
			return true;
		}

		bool insert(const NODE& item) {
			const Rect<TYPE>& area = rect(item);
			if (!getRect().contains(area)) {
				return false;
			}

			if (_nodes.empty()) {
				createNodes();
			}

			for (QuadTreeNode& node : _nodes) {
				if (!node.insert(item)) {
					continue;
				}
				return true;
			}

			_contents.push_back(item);
			return true;
		}

		inline bool isEmpty() const {
			return _nodes.empty() && _contents.empty();
		}

		void query(const Rect<TYPE>& queryArea, Contents& results) const {
			for (const NODE& item : _contents) {
				const Rect<TYPE>& area = rect(item);
				if (queryArea.intersectsWith(area)) {
					results.push_back(item);
				}
			}

			for (const QuadTreeNode& node : _nodes) {
				if (node.isEmpty()) {
					continue;
				}

				if (node.getRect().contains(queryArea)) {
					node.query(queryArea, results);
					// the queried area is completely part of the node
					break;
				}

				if (queryArea.contains(node.getRect())) {
					node.getAllContents(results);
					// the whole node content is part of the query
					continue;
				}

				if (node.getRect().intersectsWith(queryArea)) {
					node.query(queryArea, results);
				}
			}
		}
	};

	QuadTreeNode _root;
	// dirty flag can be used for query caches
	bool _dirty;
public:
	QuadTree(const Rect<TYPE>& rectangle, int maxDepth = 10) :
			_root(rectangle, maxDepth, 0), _dirty(false) {
	}

	inline int count() const {
		return _root.count();
	}

	inline bool insert(const NODE& item) {
		if (_root.insert(item)) {
			_dirty = true;
			return true;
		}
		return false;
	}

	inline bool remove(const NODE& item) {
		if (_root.remove(item)) {
			_dirty = true;
			return true;
		}
		return false;
	}

	inline void query(const Rect<TYPE>& area, Contents& results) const {
		core_trace_scoped(QuadTreeQuery);
		_root.query(area, results);
	}

	void clear() {
		_dirty = true;
		_root._contents.clear();
		_root._nodes.clear();
		_root._nodes.reserve(4);
	}

	inline void markAsClean() {
		_dirty = false;
	}

	inline bool isDirty() {
		return _dirty;
	}

	inline void getContents(Contents& results) const {
		results.clear();
		results.reserve(count());
		_root.getAllContents(results);
	}
};

#define CACHE 1
template<class NODE, typename TYPE>
class QuadTreeCache {
private:
	QuadTree<NODE, TYPE>& _tree;
#if CACHE
	std::unordered_map<Rect<TYPE>, typename QuadTree<NODE, TYPE>::Contents> _cache;
#endif
public:
	QuadTreeCache(QuadTree<NODE, TYPE>& tree) :
			_tree(tree) {
	}

	inline void clear() {
#if CACHE
		_cache.clear();
#endif
	}

	inline bool query(const Rect<TYPE>& area, typename QuadTree<NODE, TYPE>::Contents& contents) {
#if CACHE
		if (_tree.isDirty()) {
			_tree.markAsClean();
			clear();
		}
		// TODO: normalize to quad tree cells to improve the cache hits
		auto iter = _cache.find(area);
		if (iter != _cache.end()) {
			contents = iter->second;
			return true;
		}
		_tree.query(area, contents);
		_cache.insert(std::make_pair(area, contents));
#else
		_tree.query(area, contents);
#endif
		return false;
	}
};

#undef CACHE

}
