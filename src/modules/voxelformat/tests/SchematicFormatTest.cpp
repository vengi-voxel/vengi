/**
 * @file
 */

#include "voxelformat/private/minecraft/SchematicFormat.h"
#include "AbstractFormatTest.h"
#include "util/VarUtil.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class SchematicFormatTest : public AbstractFormatTest {};

TEST_F(SchematicFormatTest, testLoadLitematic) {
	testLoad("test.litematic");
}

TEST_F(SchematicFormatTest, testLoadVikingIsland) {
	// https://www.planetminecraft.com/project/viking-island-4911284/
	testLoad("viking_island.schematic");
}

TEST_F(SchematicFormatTest, testLoadStructory) {
	// https://www.planetminecraft.com/data-pack/structory/
	testLoad("brick_chimney_1.nbt");
}

TEST_F(SchematicFormatTest, testLoadAxiom) {
	testLoad("schematic.bp");
}

TEST_F(SchematicFormatTest, testSaveSmallVoxel) {
	SchematicFormat f;
	util::ScopedVarChange scoped(cfg::VoxformatMerge, "true");
	core::String filename = "minecraft-smallvolumesavetest.schematic";
	SCOPED_TRACE(filename.c_str());
	int mins = 0;
	int maxs = 3;
	const voxel::Region region(mins, maxs);
	voxel::RawVolume original(region);

	original.setVoxel(mins, mins, mins, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	original.setVoxel(maxs, maxs, maxs, voxel::createVoxel(voxel::VoxelType::Generic, 2));
	original.setVoxel(region.getCenter(), voxel::createVoxel(voxel::VoxelType::Generic, 2));

	palette::Palette pal;
	pal.minecraft();

	scenegraph::SceneGraph sceneGraph;
	io::ArchivePtr archive = helper_archive();
	{
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(&original);
		node.setPalette(pal);
		sceneGraph.emplace(core::move(node));
		ASSERT_TRUE(f.save(sceneGraph, filename, archive, testSaveCtx)) << "Could not save the scene graph";
	}

	scenegraph::SceneGraph sceneGraphLoad;
	ASSERT_TRUE(f.load(filename, archive, sceneGraphLoad, testLoadCtx)) << "Failed to load the scene grpah";
	voxel::sceneGraphComparator(sceneGraph, sceneGraphLoad, voxel::ValidateFlags::All, 0.001f);
}

} // namespace voxelformat
