#pragma once

#include <vector>
#include <algorithm>
#include <unordered_map>
#include "Rect.h"

namespace core {

template<class NODE, typename TYPE>
class QuadTree {
public:
	typedef typename std::vector<NODE> Contents;
private:
	class QuadTreeNode;
	typedef typename std::vector<QuadTreeNode> Nodes;
	class QuadTreeNode {
		friend class QuadTree;
	private:
		const int _maxDepth;
		int _depth;
		const Rect<TYPE> _area;
		Contents _contents;
		Nodes _nodes;

		QuadTreeNode(const Rect<TYPE>& bounds, int maxDepth, int depth) :
				_maxDepth(maxDepth), _depth(depth), _area(bounds) {
			_nodes.reserve(4);
		}

		void createNodes() {
			if (_depth >= _maxDepth) {
				return;
			}

			const std::array<Rect<TYPE>, 4>& subareas = _area.split();
			_nodes.push_back(QuadTreeNode(subareas[0], _maxDepth, _depth + 1));
			_nodes.push_back(QuadTreeNode(subareas[1], _maxDepth, _depth + 1));
			_nodes.push_back(QuadTreeNode(subareas[2], _maxDepth, _depth + 1));
			_nodes.push_back(QuadTreeNode(subareas[3], _maxDepth, _depth + 1));
		}

		inline int count() const {
			int count = 0;
			for (auto& node : _nodes) {
				count += node.count();
			}
			count += _contents.size();
			return count;
		}

		inline const Rect<TYPE>& getRect() const {
			return _area;
		}

		inline const Contents& getContents() const {
			return _contents;
		}

		Contents getAllContents() const {
			Contents results;

			for (auto& node : _nodes) {
				const Contents& childNodes = node.getAllContents();
				results.insert(results.begin(), childNodes.begin(), childNodes.end());
			}

			results.insert(results.begin(), _contents.begin(), _contents.end());
			return results;
		}

		bool remove(const NODE& item) {
			const Rect<TYPE>& area = item.getRect();
			if (!_area.contains(area)) {
				return false;
			}
			for (auto& node : _nodes) {
				if (!node._area.contains(area))
					continue;
				return node.remove(item);
			}

			auto i = std::find(_contents.begin(), _contents.end(), item);
			if (i == _contents.end())
				return false;
			_contents.erase(i);
			return true;
		}

		bool insert(const NODE& item) {
			const Rect<TYPE>& area = item.getRect();
			if (!_area.contains(area)) {
				return false;
			}

			if (_nodes.empty()) {
				createNodes();
			}

			for (auto& node : _nodes) {
				if (!node._area.contains(area))
					continue;
				node.insert(item);
				return true;
			}

			_contents.push_back(item);
			return true;
		}

		inline bool isEmpty() const {
			if (_nodes.empty() && _contents.empty())
				return true;

			return false;
		}

		Contents query(const Rect<TYPE>& queryArea) const {
			Contents results;

			for (auto& item : _contents) {
				const Rect<TYPE>& area = item.getRect();
				if (queryArea.intersectsWith(area)) {
					results.push_back(item);
				}
			}

			for (auto& node : _nodes) {
				if (node.isEmpty()) {
					continue;
				}

				if (node._area.contains(queryArea)) {
					const Contents& contents = node.query(queryArea);
					if (!contents.empty()) {
						results.reserve(results.size() + contents.size());
						results.insert(results.end(), contents.begin(), contents.end());
					}
					// the queried area is completely part of the node
					break;
				}

				if (queryArea.contains(node._area)) {
					const Contents& contents = node.getAllContents();
					if (!contents.empty()) {
						results.reserve(results.size() + contents.size());
						results.insert(results.end(), contents.begin(), contents.end());
					}
					// the whole node content is part of the query
					continue;
				}

				if (node._area.intersectsWith(queryArea)) {
					const Contents& contents = node.query(queryArea);
					if (!contents.empty()) {
						results.reserve(results.size() + contents.size());
						results.insert(results.end(), contents.begin(), contents.end());
					}
				}
			}
			return results;
		}

		inline std::string indent() const {
			std::string indent;
			for (int i = 0; i < _depth; ++i) {
				indent.push_back(' ');
				indent.push_back(' ');
			}
			return indent;
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

	inline Contents query(const Rect<TYPE>& area) const {
		return _root.query(area);
	}

	void clear() {
		auto size = _root._contents.size();
		_dirty = true;
		_root._contents.clear();
		_root._contents.reserve(size);
		_root._nodes.clear();
		_root._nodes.reserve(4);
	}

	inline void markAsClean() {
		_dirty = false;
	}

	inline bool isDirty() {
		return _dirty;
	}

	inline Contents getContents() const {
		return _root.getAllContents();
	}
};

#define CACHE 0
template<class NODE, typename TYPE>
class QuadTreeCache {
private:
	QuadTree<NODE, TYPE>& _tree;
#if CACHE
	std::unordered_map<typename Rect<TYPE>, typename QuadTree<NODE, TYPE>::Contents> _cache;
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

#if CACHE
	inline const typename QuadTree<NODE, TYPE>::Contents& query(const Rect<TYPE>& area) {
		if (_tree.isDirty()) {
			_tree.markAsClean();
			clear();
		}
		auto iter = _cache.find(area);
		if (iter != _cache.end())
			return iter->second;
		auto insertIter = _cache.insert(std::make_pair(area, std::move(_tree.query(area))));
		return insertIter->first;
#else
	inline typename QuadTree<NODE, TYPE>::Contents query(const Rect<TYPE>& area) {
		return _tree.query(area);
#endif
	}
};

}
