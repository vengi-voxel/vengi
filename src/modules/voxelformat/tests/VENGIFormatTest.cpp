/**
 * @file
 */

#include "voxelformat/private/vengi/VENGIFormat.h"
#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "palette/Palette.h"
#include "scenegraph/IKConstraint.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/VarUtil.h"
#include "voxel/ChunkMesh.h"
#include "voxel/Mesh.h"
#include "voxel/RawVolume.h"
#include "voxel/SurfaceExtractor.h"
#include "voxel/Voxel.h"
#include "voxelformat/private/mesh/GLTFFormat.h"
#include "voxelformat/tests/TestHelper.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class VENGIFormatTest : public AbstractFormatTest {};

TEST_F(VENGIFormatTest, testMeshChrKnightGreedyTexture) {
	util::ScopedVarChange var(cfg::VoxformatMeshMode, (int)voxel::SurfaceExtractionType::GreedyTexture);
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "chr_knight.vengi", 19);
	if (HasFatalFailure()) {
		return;
	}

	struct Expected {
		uint32_t vertices;
		uint32_t indices;
	} expected[] = {{24, 36},  {24, 36},  {212, 318}, {136, 204}, {88, 132}, {68, 102}, {64, 96},
					{68, 102}, {64, 96},  {24, 36},   {68, 102},  {24, 36},  {40, 60},  {24, 36},
					{24, 36},  {68, 102}, {24, 36},   {40, 60},   {24, 36}};
	int nodes = 0;
	for (const auto &e : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = e->value;
		if (node.type() != scenegraph::SceneGraphNodeType::Model) {
			continue;
		}
		const voxel::RawVolume *v = node.volume();
		voxel::ChunkMesh mesh;
		voxel::SurfaceExtractionContext ctx = voxel::createContext(voxel::SurfaceExtractionType::GreedyTexture, v,
																   v->region(), node.palette(), mesh, glm::ivec3(0));
		voxel::extractSurface(ctx);
		EXPECT_EQ(expected[nodes].vertices, mesh.mesh[0].getVertexVector().size());
		EXPECT_EQ(expected[nodes].indices, mesh.mesh[0].getIndexVector().size());
		nodes++;
	}
}

TEST_F(VENGIFormatTest, testMeshCubeGreedyTexture) {
	util::ScopedVarChange var(cfg::VoxformatMeshMode, (int)voxel::SurfaceExtractionType::GreedyTexture);
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "cube.vengi", 1);
	if (HasFatalFailure()) {
		return;
	}

	scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(node, nullptr);

	const voxel::RawVolume *v = node->volume();
	voxel::ChunkMesh mesh;
	voxel::SurfaceExtractionContext ctx = voxel::createContext(voxel::SurfaceExtractionType::GreedyTexture, v,
															   v->region(), node->palette(), mesh, glm::ivec3(0));
	voxel::extractSurface(ctx);
	uint32_t expectedFaces = 6u;
	const voxel::VertexArray &vertices = mesh.mesh[0].getVertexVector();
	const voxel::IndexArray &indices = mesh.mesh[0].getIndexVector();
	ASSERT_EQ(expectedFaces * 4u, vertices.size());
	ASSERT_EQ(expectedFaces * 6u, indices.size());

	// clang-format off
	const voxel::IndexArray::value_type expectedIndices[] = {
		0, 1, 2, 0, 2, 3,
		4, 5, 6, 4, 6, 7,
		8, 9, 10, 8, 10, 11,
		12, 13, 14, 12, 14, 15,
		16, 17, 18, 16, 18, 19,
		20, 21, 22, 20, 22, 23
	};
	const glm::vec3 expectedPositions[] = {
		{0, 0, 0}, {0, 0, 3}, {0, 3, 3}, {0, 3, 0},
		{3, 0, 0}, {3, 3, 0}, {3, 3, 3}, {3, 0, 3},
		{0, 0, 0}, {3, 0, 0}, {3, 0, 3}, {0, 0, 3},
		{0, 3, 0}, {0, 3, 3}, {3, 3, 3}, {3, 3, 0},
		{0, 0, 0}, {0, 3, 0}, {3, 3, 0}, {3, 0, 0},
		{0, 0, 3}, {3, 0, 3}, {3, 3, 3}, {0, 3, 3}
	};
	// clang-format on

	for (uint32_t i = 0; i < indices.size(); i++) {
		ASSERT_EQ(expectedIndices[i], indices[i]) << "index " << i;
	}
	for (uint32_t i = 0; i < vertices.size(); i++) {
		ASSERT_EQ(expectedPositions[i], vertices[i].position) << "vertex " << i;
	}
}

TEST_F(VENGIFormatTest, testMeshCubeGreedyTextureGLTF) {
	util::ScopedVarChange var(cfg::VoxformatMeshMode, (int)voxel::SurfaceExtractionType::GreedyTexture);
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "cube.vengi", 1);
	if (HasFatalFailure()) {
		return;
	}
	const io::ArchivePtr &archive = helper_filesystemarchive();

	const core::String modeFilename = "cube.gltf";
	GLTFFormat format;
	ASSERT_TRUE(format.save(sceneGraph, modeFilename, archive, testSaveCtx))
		<< "Could not save " << modeFilename.c_str();

	scenegraph::SceneGraph sceneGraphLoad;
	ASSERT_TRUE(format.load(modeFilename, archive, sceneGraphLoad, testLoadCtx))
		<< "Could not load " << modeFilename.c_str();
	voxel::sceneGraphComparator(sceneGraph, sceneGraphLoad, voxel::ValidateFlags::Mesh);
	scenegraph::SceneGraphNode *node = sceneGraphLoad.firstModelNode();
	ASSERT_NE(node, nullptr);
	const voxel::RawVolume *v = node->volume();
	ASSERT_EQ(27, voxelutil::countVoxels(*v));
}

TEST_F(VENGIFormatTest, testSaveSmallVolume) {
	VENGIFormat f;
	testSaveSmallVolume("testSaveSmallVolume.vengi", &f);
}

TEST_F(VENGIFormatTest, testSaveLoadVoxel) {
	VENGIFormat f;
	testSaveLoadVoxel("testSaveLoadVoxel.vengi", &f);
}

TEST_F(VENGIFormatTest, testSaveLoadIKConstraint) {
	VENGIFormat f;
	palette::Palette pal;
	pal.magicaVoxel();
	const voxel::Region &region = voxel::Region::fromSize(2);
	voxel::RawVolume original(region);
	ASSERT_TRUE(original.setVoxel(0, 0, 0, voxel::createVoxel(pal, 1)));

	scenegraph::SceneGraph sceneGraphSave;
	int effectorNodeId;
	{
		scenegraph::SceneGraphNode effectorNode(scenegraph::SceneGraphNodeType::Model);
		effectorNode.setUnownedVolume(&original);
		effectorNode.setPalette(pal);
		effectorNode.setName("effector-node");
		effectorNodeId = sceneGraphSave.emplace(core::move(effectorNode));
		ASSERT_NE(InvalidNodeId, effectorNodeId);
	}
	{
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setUnownedVolume(&original);
		node.setPalette(pal);
		node.setName("ik-node");

		scenegraph::IKConstraint ik;
		ik.effectorNodeId = effectorNodeId;
		ik.rollMin = -1.5f;
		ik.rollMax = 2.0f;
		ik.visible = false;
		ik.anchor = true;
		scenegraph::IKConstraint::RadiusConstraint swing;
		swing.center = glm::vec2(0.5f, 1.0f);
		swing.radius = 0.75f;
		ik.swingLimits.push_back(swing);
		node.setIkConstraint(ik);

		sceneGraphSave.emplace(core::move(node));
	}

	const io::ArchivePtr &archive = helper_archive();
	ASSERT_TRUE(f.saveGroups(sceneGraphSave, "testIK.vengi", archive, testSaveCtx));

	scenegraph::SceneGraph sceneGraphLoad;
	ASSERT_TRUE(f.loadGroups("testIK.vengi", archive, sceneGraphLoad, testLoadCtx));

	// find the ik-node by name
	const scenegraph::SceneGraphNode *loadedIKNode = nullptr;
	const scenegraph::SceneGraphNode *loadedEffectorNode = nullptr;
	for (auto iter = sceneGraphLoad.beginModel(); iter != sceneGraphLoad.end(); ++iter) {
		if ((*iter).name() == "ik-node") {
			loadedIKNode = &*iter;
		} else if ((*iter).name() == "effector-node") {
			loadedEffectorNode = &*iter;
		}
	}
	ASSERT_NE(nullptr, loadedIKNode);
	ASSERT_NE(nullptr, loadedEffectorNode);
	ASSERT_TRUE(loadedIKNode->hasIKConstraint());
	const scenegraph::IKConstraint *loadedIK = loadedIKNode->ikConstraint();
	ASSERT_NE(nullptr, loadedIK);
	EXPECT_EQ(loadedEffectorNode->id(), loadedIK->effectorNodeId);
	EXPECT_FLOAT_EQ(-1.5f, loadedIK->rollMin);
	EXPECT_FLOAT_EQ(2.0f, loadedIK->rollMax);
	EXPECT_FALSE(loadedIK->visible);
	EXPECT_TRUE(loadedIK->anchor);
	ASSERT_EQ(1u, loadedIK->swingLimits.size());
	EXPECT_FLOAT_EQ(0.5f, loadedIK->swingLimits[0].center.x);
	EXPECT_FLOAT_EQ(1.0f, loadedIK->swingLimits[0].center.y);
	EXPECT_FLOAT_EQ(0.75f, loadedIK->swingLimits[0].radius);
}

} // namespace voxelformat
