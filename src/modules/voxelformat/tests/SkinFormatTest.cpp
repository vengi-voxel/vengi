/**
 * @file
 */

#include "voxelformat/private/minecraft/SkinFormat.h"
#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "util/VarUtil.h"
#include "gtest/gtest.h"

namespace voxelformat {

struct Params {
	bool groups;
	bool transform;
	bool mergeFaces;
	voxel::ValidateFlags flags;
};

class Param : public AbstractFormatTest, public ::testing::WithParamInterface<Params> {};

TEST_P(Param, loadSave) {
	SkinFormat src;
	SkinFormat target;
	const Params &params = GetParam();
	util::ScopedVarChange groups(cfg::VoxformatSkinAddGroups, params.groups);
	util::ScopedVarChange transform(cfg::VoxformatSkinApplyTransform, params.transform);
	util::ScopedVarChange mergeFaces(cfg::VoxformatSkinMergeFaces, params.mergeFaces);
	testLoadSaveAndLoadSceneGraph("minecraft-skin.png", src, "minecraft-skin-test.mcskin", target, params.flags);
}

INSTANTIATE_TEST_SUITE_P(
	SkinFormatTest,
	Param,
	::testing::Values(
		Params{true, true, false, voxel::ValidateFlags::All},
		Params{false, true, false, voxel::ValidateFlags::All},
		Params{false, false, false, voxel::ValidateFlags::All},
		Params{true, true, true, voxel::ValidateFlags::Transform | voxel::ValidateFlags::SceneGraphModels},
		Params{false, true, true, voxel::ValidateFlags::Transform | voxel::ValidateFlags::SceneGraphModels},
		Params{false, false, true, voxel::ValidateFlags::Transform | voxel::ValidateFlags::SceneGraphModels}
	),
    [](const testing::TestParamInfo<Params>& nfo) {
      std::string name;
	  if (nfo.param.groups) {
		name += "groups";
	  } else {
		name += "nogroups";
	  }
	  if (nfo.param.transform) {
		name += "_transform";
	  } else {
		name += "_notransform";
	  }
	  if (nfo.param.mergeFaces) {
		name += "_mergefaces";
	  } else {
		name += "_nomergefaces";
	  }
      return name;
    }
);

} // namespace voxelformat
