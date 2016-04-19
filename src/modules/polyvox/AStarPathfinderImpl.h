#pragma once

#include "core/Common.h"

#include <algorithm>
#include <limits> //For numeric_limits
#include <set>
#include <vector>

namespace PolyVox {
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
			gVal(std::numeric_limits<float>::quiet_NaN()) //Initilise with NaNs so that we will
					, hVal(std::numeric_limits<float>::quiet_NaN()) //know if we forget to set these properly.
					, parent(0) {
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

	float f(void) const {
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
	void clear(void) {
		open.clear();
	}

	bool empty(void) const {
		return open.empty();
	}

	void insert(AllNodesContainer::iterator node) {
		open.push_back(node);
		push_heap(open.begin(), open.end(), NodeSort());
	}

	AllNodesContainer::iterator getFirst(void) {
		return open[0];
	}

	void removeFirst(void) {
		pop_heap(open.begin(), open.end(), NodeSort());
		open.pop_back();
	}

	void remove(iterator iterToRemove) {
		open.erase(iterToRemove);
		make_heap(open.begin(), open.end(), NodeSort());
	}

	iterator begin(void) {
		return open.begin();
	}

	iterator end(void) {
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
	void clear(void) {
		closed.clear();
	}

	void insert(AllNodesContainer::iterator node) {
		closed.insert(node);
	}

	void remove(iterator iterToRemove) {
		closed.erase(iterToRemove);
	}

	iterator begin(void) {
		return closed.begin();
	}

	iterator end(void) {
		return closed.end();
	}

	iterator find(AllNodesContainer::iterator node) {
		iterator iter = std::find(closed.begin(), closed.end(), node);
		return iter;
	}

private:
	std::set<AllNodesContainer::iterator, AllNodesContainerIteratorComparator> closed;
};

}
