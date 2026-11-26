/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "util/VarUtil.h"
#include "scenegraph/SceneGraph.h"
#include "voxelformat/tests/TestHelper.h"
#include "voxelformat/private/qubicle/QBFormat.h"
#include "io/MemoryArchive.h"
#include "math/Vec3f.h"

namespace voxelformat {

class QBFormatTest: public AbstractFormatTest {
};

TEST_F(QBFormatTest, testPalettePerModel) {
	QBFormat format;
	testModelPaletteRoundTrip("model-palettes.qb", format);
	util::ScopedVarChange compressed(cfg::VoxformatQBSaveCompressed, false);
	testModelPaletteRoundTrip("matrix-palettes-uncompressed.qb", format);
	util::ScopedVarChange leftHanded(cfg::VoxformatQBSaveLeftHanded, false);
	testModelPaletteRoundTrip("matrix-palettes-right-handed.qb", format);
}

TEST_F(QBFormatTest, testLoad) {
	testLoad("qubicle.qb", 10);
}

TEST_F(QBFormatTest, testLoadRGB) {
	testRGB("rgb.qb");
}

TEST_F(QBFormatTest, testLoadRGBSmall) {
	testRGBSmall("rgb_small.qb");
}

TEST_F(QBFormatTest, testLoadRGBSmallSaveLoad) {
	testRGBSmallSaveLoad("rgb_small.qb");
}

TEST_F(QBFormatTest, testSaveSingleVoxel) {
	QBFormat f;
	testSaveSingleVoxel("qubicle-singlevoxelsavetest.qb", &f);
}

TEST_F(QBFormatTest, testSaveSmallVoxel) {
	QBFormat f;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Palette | voxel::ValidateFlags::SceneGraphModelsParent);
	testSaveLoadVoxel("qubicle-smallvolumesavetest.qb", &f, 0, 1, flags);
}

TEST_F(QBFormatTest, testSaveMultipleModels) {
	QBFormat f;
	testSaveMultipleModels("qubicle-multiplemodelsavetest.qb", &f);
}

TEST_F(QBFormatTest, testSavePivotedGridWithParentAndNonzeroLowerCorner) {
	scenegraph::SceneGraph graph;
	scenegraph::SceneGraphNode parent(scenegraph::SceneGraphNodeType::Group);
	scenegraph::SceneGraphTransform parentTransform;
	parentTransform.setLocalTranslation(math::Vec3f(10, 20, 30));
	parent.setTransform(0, parentTransform);
	const int parentId = graph.emplace(core::move(parent));
	ASSERT_NE(InvalidNodeId, parentId);
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	voxel::RawVolume *volume = new voxel::RawVolume(voxel::Region(-2, -3, -4, 1, 0, -1));
	palette::Palette palette;
	palette.setColor(0, color::RGBA(255, 0, 0, 255));
	volume->setVoxel(-2, -3, -4, voxel::createVoxel(palette, 0));
	volume->setVoxel(1, 0, -1, voxel::createVoxel(palette, 0));
	node.setVolume(volume);
	node.setPalette(palette);
	node.setPivot(math::Vec3f(0.5f));
	scenegraph::SceneGraphTransform transform;
	transform.setLocalTranslation(math::Vec3f(2, 3, 4));
	node.setTransform(0, transform);
	ASSERT_NE(InvalidNodeId, graph.emplace(core::move(node), parentId));
	graph.updateTransforms();
	QBFormat format;
	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	ASSERT_TRUE(format.save(graph, "pivot.qb", archive, testSaveCtx));
	scenegraph::SceneGraph loaded;
	ASSERT_TRUE(format.load("pivot.qb", archive, loaded, testLoadCtx));
	loaded.updateTransforms();
	ASSERT_NE(nullptr, loaded.firstModelNode());
	EXPECT_VEC_NEAR(math::Vec3f(8, 18, 28), loaded.firstModelNode()->transform(0).worldTranslation(), 0.0001f);
	voxel::occupiedWorldComparator(graph, loaded, false);
}

}
