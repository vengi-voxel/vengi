/**
 * @file
 */

#include "voxelformat/FormatConfig.h"
#include "core/ConfigVar.h"
#include "core/Var.h"
#include <gtest/gtest.h>

namespace voxelformat {

class FormatConfigTest : public testing::Test {
protected:
	void SetUp() override {
		FormatConfig::init();
	}
};

TEST_F(FormatConfigTest, testNumericMinMax) {
	const core::VarPtr &voxelSize = core::getVar(cfg::VoxformatVoxelSize);
	EXPECT_EQ("Voxel size", voxelSize->title());
	ASSERT_TRUE(voxelSize->hasMinMax());
	EXPECT_EQ(0, voxelSize->intMinValue());
	EXPECT_EQ(1024, voxelSize->intMaxValue());
}

TEST_F(FormatConfigTest, testPathCvars) {
	const core::VarPtr &palette = core::getVar(cfg::VoxelPalette);
	EXPECT_EQ(core::VarType::Path, palette->type());

	const core::VarPtr &ldraw = core::getVar(cfg::VoxformatLDrawDir);
	EXPECT_EQ(core::VarType::Directory, ldraw->type());

	const core::VarPtr &tex = core::getVar(cfg::VoxformatTexturePath);
	EXPECT_EQ(core::VarType::Directory, tex->type());
}

} // namespace voxelformat
