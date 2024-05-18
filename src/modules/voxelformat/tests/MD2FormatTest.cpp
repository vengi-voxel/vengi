/**
 * @file
 */

#include "AbstractFormatTest.h"

namespace voxelformat {

class MD2FormatTest : public AbstractFormatTest {};

TEST_F(MD2FormatTest, testVoxelize) {
	// public domain model from https://github.com/ufoaiorg/ufoai/blob/master/base/models/objects/barrel_fuel/barrel_fuel.md2 (Nobiax/yughues, Open Game Art (http://openga)
	testLoad("fuel_can.md2");
}

} // namespace voxelformat
