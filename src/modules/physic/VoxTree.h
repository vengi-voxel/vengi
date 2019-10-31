/**
 * @file
 */

#pragma once

#include "core/GLM.h"
#include "VoxTreeNode.h"
#include "voxel/Voxel.h"
#include "core/NonCopyable.h"

namespace physic {

template<class Volume>
class VoxTree : public core::NonCopyable {
private:
	void populate(VoxTreeNode *node) const;
	void findBestSplit(VoxTreeNode *node, int &pos, int &quality, int axis) const;
	void eval(VoxTreeNode *node) const;

	inline voxel::VoxelType at(int x, int y, int z) const {
		return _volume->voxel(x, y, z).getMaterial();
	}

	VoxTreeNode *_root;
	const Volume *_volume;
	const glm::ivec3 _mins;
	const glm::ivec3 _maxs;

public:
	VoxTree(const Volume *volume, const glm::ivec3& mins, const glm::ivec3& maxs);
	virtual ~VoxTree();

	inline VoxTreeNode* root() {
		return _root;
	}
};

template<class Volume>
VoxTree<Volume>::VoxTree(const Volume *volume, const glm::ivec3& mins, const glm::ivec3& maxs) :
		_root(0), _volume(volume), _mins(mins), _maxs(maxs) {
	_root = new VoxTreeNode(mins, maxs);
	populate(_root);
}

template<class Volume>
VoxTree<Volume>::~VoxTree() {
	delete _root;
}

template<class Volume>
void VoxTree<Volume>::populate(VoxTreeNode *node) const {
	eval(node);

	if (node->_value == VoxTreeNode::nodeMixed) {
		int xpos;
		int xqual;
		int ypos;
		int yqual;
		int zpos;
		int zqual;
		findBestSplit(node, xpos, xqual, 0);
		findBestSplit(node, ypos, yqual, 1);
		findBestSplit(node, zpos, zqual, 2);

		glm::ivec3 split;
		if (xqual > yqual && xqual > zqual) {
			split = node->_maxs;
			split.x = xpos;
			node->_children[0] = new VoxTreeNode(node->_mins, split);
			split = node->_mins;
			split.x = xpos + 1;
			node->_children[1] = new VoxTreeNode(split, node->_maxs);
		} else if (yqual > zqual) {
			split = node->_maxs;
			split.y = ypos;
			node->_children[0] = new VoxTreeNode(node->_mins, split);
			split = node->_mins;
			split.y = ypos + 1;
			node->_children[1] = new VoxTreeNode(split, node->_maxs);
		} else {
			split = node->_maxs;
			split.z = zpos;
			node->_children[0] = new VoxTreeNode(node->_mins, split);
			split = node->_mins;
			split.z = zpos + 1;
			node->_children[1] = new VoxTreeNode(split, node->_maxs);
		}

		populate(node->_children[0]);
		populate(node->_children[1]);
	}
}

template<class Volume>
void VoxTree<Volume>::findBestSplit(VoxTreeNode *node, int &pos, int &quality, int axis) const {
	quality = 0;
	pos = 0;
	const int a0 = axis;
	const int a1 = (axis + 1) % 3;
	const int a2 = (axis + 2) % 3;
	glm::ivec3 coord(0, 0, 0);
	typename Volume::Sampler sampler(_volume);
	for (coord[a0] = node->_mins[a0]; coord[a0] < node->_maxs[a0]; ++coord[a0]) {
		int qual = 0;
		for (coord[a1] = node->_mins[a1]; coord[a1] <= node->_maxs[a1]; ++coord[a1]) {
			for (coord[a2] = node->_mins[a2]; coord[a2] <= node->_maxs[a2]; ++coord[a2]) {
				sampler.setPosition(coord);
				const voxel::VoxelType v1 = sampler.voxel().getMaterial();
				if (a0 == 0) {
					sampler.movePositiveX();
				} else if (a0 == 1) {
					sampler.movePositiveY();
				} else {
					sampler.movePositiveZ();
				}
				const voxel::VoxelType v2 = sampler.voxel().getMaterial();
				if (voxel::isAir(v1) != voxel::isAir(v2)) {
					++qual;
				}
			}
		}
		if (qual > quality) {
			pos = coord[a0];
			quality = qual;
		}
	}
}

template<class Volume>
void VoxTree<Volume>::eval(VoxTreeNode *node) const {
	bool foundSet = false;
	bool foundUnset = false;
	for (int z = node->_mins.z; z <= node->_maxs.z; ++z) {
		for (int y = node->_mins.y; y <= node->_maxs.y; ++y) {
			for (int x = node->_mins.x; x <= node->_maxs.x; ++x) {
				if (voxel::isAir(at(x, y, z))) {
					foundUnset = true;
				} else {
					foundSet = true;
				}
				if (foundSet && foundUnset) {
					node->_value = VoxTreeNode::nodeMixed;
					return;
				}
			}
		}
	}
	if (foundSet) {
		node->_value = VoxTreeNode::nodeFull;
	} else {
		node->_value = VoxTreeNode::nodeEmpty;
	}
}

}
