/**
 * @file
 */

#pragma once

#include "voxelgenerator/TreeContext.h"

namespace voxedit {

/**
 * @brief Panel for the tree generator
 */
class TreePanel {
private:
	voxelgenerator::TreeContext _treeGeneratorContext;

	void switchTreeType(voxelgenerator::TreeType treeType);
public:
	TreePanel();
	void update(const char *title);
};

}
