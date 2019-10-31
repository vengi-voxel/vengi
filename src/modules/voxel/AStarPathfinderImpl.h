/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <algorithm>
#include <limits> //For numeric_limits
#include <set>
#include <vector>

namespace voxel {

class OpenNodesContainer;
class ClosedNodesContainer;

/// The Connectivity of a voxel determines how many neighbours it has.
enum Connectivity {
	/// Each voxel has six neighbours, which are those sharing a face.
	SixConnected,
	/// Each voxel has 18 neighbours, which are those sharing a face or an edge.
	EighteenConnected,
	/// Each voxel has 26 neighbours, which are those sharing a face, edge, or corner.
	TwentySixConnected
};

struct Node {
	Node(int x, int y, int z) :
			// Initialise with NaNs so that we will know if we forget to set these properly.
			gVal(std::numeric_limits<float>::quiet_NaN()), hVal(std::numeric_limits<float>::quiet_NaN()), parent(nullptr) {
		position = {x, y, z};
	}

	bool operator==(const Node& rhs) const {
		return position == rhs.position;
	}

	bool operator<(const Node& rhs) const {
		if (position.x < rhs.position.x)
			return true;
		if (rhs.position.x < position.x)
			return false;

		if (position.y < rhs.position.y)
			return true;
		if (rhs.position.y < position.y)
			return false;

		if (position.z < rhs.position.z)
			return true;
		if (rhs.position.z < position.z)
			return false;

		return false;
	}

	glm::ivec3 position;
	float gVal;
	float hVal;
	Node* parent;

	inline float f() const {
		return gVal + hVal;
	}
};

typedef std::set<Node> AllNodesContainer;

class AllNodesContainerIteratorComparator {
public:
	bool operator()(const AllNodesContainer::iterator& lhs, const AllNodesContainer::iterator& rhs) const {
		return (&(*lhs)) < (&(*rhs));
	}
};

class NodeSort {
public:
	bool operator()(const AllNodesContainer::iterator& lhs, const AllNodesContainer::iterator& rhs) const {
		return lhs->f() > rhs->f();
	}
};

class OpenNodesContainer {
public:
	typedef std::vector<AllNodesContainer::iterator>::iterator iterator;

public:
	inline void clear() {
		open.clear();
	}

	inline bool empty() const {
		return open.empty();
	}

	void insert(AllNodesContainer::iterator node) {
		open.push_back(node);
		push_heap(open.begin(), open.end(), NodeSort());
	}

	inline AllNodesContainer::iterator getFirst() {
		return open[0];
	}

	void removeFirst() {
		pop_heap(open.begin(), open.end(), NodeSort());
		open.pop_back();
	}

	void remove(iterator iterToRemove) {
		open.erase(iterToRemove);
		make_heap(open.begin(), open.end(), NodeSort());
	}

	inline iterator begin() {
		return open.begin();
	}

	inline iterator end() {
		return open.end();
	}

	iterator find(AllNodesContainer::iterator node) {
		std::vector<AllNodesContainer::iterator>::iterator openIter = std::find(open.begin(), open.end(), node);
		return openIter;
	}

private:
	std::vector<AllNodesContainer::iterator> open;
};

class ClosedNodesContainer {
public:
	typedef std::set<AllNodesContainer::iterator, AllNodesContainerIteratorComparator>::iterator iterator;

public:
	inline void clear() {
		closed.clear();
	}

	inline void insert(AllNodesContainer::iterator node) {
		closed.insert(node);
	}

	inline void remove(iterator iterToRemove) {
		closed.erase(iterToRemove);
	}

	inline iterator end() {
		return closed.end();
	}

	inline iterator find(AllNodesContainer::iterator node) {
		return closed.find(node);
	}

private:
	std::set<AllNodesContainer::iterator, AllNodesContainerIteratorComparator> closed;
};

}
