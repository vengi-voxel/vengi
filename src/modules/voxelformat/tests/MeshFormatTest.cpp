/**
 * @file
 */

#include "voxelformat/MeshFormat.h"
#include "core/Color.h"
#include "io/File.h"
#include "video/ShapeBuilder.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/tests/AbstractVoxFormatTest.h"

namespace voxelformat {

class MeshFormatTest : public AbstractVoxFormatTest {};

TEST_F(MeshFormatTest, testSubdivide) {
	MeshFormat::TriCollection tinyTris;
	Tri tri;
	tri.vertices[0] = glm::vec3(-8.77272797, -11.43335, -0.154544264);
	tri.vertices[1] = glm::vec3(-8.77272701, 11.1000004, -0.154543981);
	tri.vertices[2] = glm::vec3(8.77272701, 11.1000004, -0.154543981);
	MeshFormat::subdivideTri(tri, tinyTris);
	EXPECT_EQ(1024u, tinyTris.size());
}

TEST_F(MeshFormatTest, testCalculateAABB) {
	MeshFormat::TriCollection tris;
	Tri tri;

	{
		tri.vertices[0] = glm::vec3(0, 0, 0);
		tri.vertices[1] = glm::vec3(10, 0, 0);
		tri.vertices[2] = glm::vec3(10, 0, 10);
		tris.push_back(tri);
	}

	{
		tri.vertices[0] = glm::vec3(0, 0, 0);
		tri.vertices[1] = glm::vec3(-10, 0, 0);
		tri.vertices[2] = glm::vec3(-10, 0, -10);
		tris.push_back(tri);
	}

	glm::vec3 mins, maxs;
	ASSERT_TRUE(MeshFormat::calculateAABB(tris, mins, maxs));
	EXPECT_FLOAT_EQ(mins.x, -10.0f);
	EXPECT_FLOAT_EQ(mins.y, 0.0f);
	EXPECT_FLOAT_EQ(mins.z, -10.0f);
	EXPECT_FLOAT_EQ(maxs.x, 10.0f);
	EXPECT_FLOAT_EQ(maxs.y, 0.0f);
	EXPECT_FLOAT_EQ(maxs.z, 10.0f);
}

TEST_F(MeshFormatTest, testAreAllTrisAxisAligned) {
	MeshFormat::TriCollection tris;
	Tri tri;

	{
		tri.vertices[0] = glm::vec3(0, 0, 0);
		tri.vertices[1] = glm::vec3(10, 0, 0);
		tri.vertices[2] = glm::vec3(10, 0, 10);
		tris.push_back(tri);
	}

	{
		tri.vertices[0] = glm::vec3(0, 0, 0);
		tri.vertices[1] = glm::vec3(-10, 0, 0);
		tri.vertices[2] = glm::vec3(-10, 0, -10);
		tris.push_back(tri);
	}

	EXPECT_TRUE(MeshFormat::isAxisAligned(tris));

	{
		tri.vertices[0] = glm::vec3(0, 0, 0);
		tri.vertices[1] = glm::vec3(-10, 1, 0);
		tri.vertices[2] = glm::vec3(-10, 0, -10);
		tris.push_back(tri);
	}

	EXPECT_FALSE(MeshFormat::isAxisAligned(tris));
}

TEST_F(MeshFormatTest, testVoxelizeColor) {
	class TestMesh : public MeshFormat {
	public:
		bool saveMeshes(const core::Map<int, int> &, const SceneGraph &, const Meshes &, const core::String &,
						io::SeekableWriteStream &, const glm::vec3 &, bool, bool, bool) override {
			return false;
		}
		void voxelize(voxelformat::SceneGraph &sceneGraph, const MeshFormat::TriCollection &tris) {
			voxelizeNode("test", sceneGraph, tris);
		}
	};

	TestMesh mesh;
	MeshFormat::TriCollection tris;

	video::ShapeBuilder b;
	voxelformat::SceneGraph sceneGraph;

	voxel::getPalette().nippon();
	const core::RGBA nipponRed = voxel::getPalette().colors[37];
	b.setPosition({10.0f, 0.0f, 10.0f});
	b.setColor(core::Color::fromRGBA(nipponRed));
	b.pyramid({10.0f, 10.0f, 10.0f});

	const video::ShapeBuilder::Indices &indices = b.getIndices();
	const video::ShapeBuilder::Vertices &vertices = b.getVertices();

	// color of the tip is green
	video::ShapeBuilder::Colors colors = b.getColors();
	const core::RGBA nipponGreen = voxel::getPalette().colors[145];
	colors[0] = core::Color::fromRGBA(nipponGreen);

	const int n = (int)indices.size();
	for (int i = 0; i < n; i += 3) {
		Tri tri;
		for (int j = 0; j < 3; ++j) {
			tri.vertices[j] = vertices[indices[i + j]];
			tri.color[j] = core::Color::getRGBA(colors[indices[i + j]]);
		}
		tris.push_back(tri);
	}
	mesh.voxelize(sceneGraph, tris);
	SceneGraphNode *node = sceneGraph.findNodeByName("test");
	ASSERT_NE(nullptr, node);
	voxel::getPalette() = node->palette();
	const voxel::RawVolume *v = node->volume();
	EXPECT_LT(core::Color::getDistance(nipponRed, node->palette().colors[v->voxel(0, 0, 0).getColor()]), 0.00004f);
	EXPECT_LT(core::Color::getDistance(nipponRed, node->palette().colors[v->voxel(20, 0, 20).getColor()]), 0.00004f);
	EXPECT_LT(core::Color::getDistance(nipponRed, node->palette().colors[v->voxel(0, 0, 20).getColor()]), 0.00004f);
	EXPECT_LT(core::Color::getDistance(nipponRed, node->palette().colors[v->voxel(20, 0, 0).getColor()]), 0.00004f);
	EXPECT_LT(core::Color::getDistance(nipponGreen, node->palette().colors[v->voxel(10, 10, 10).getColor()]), 0.00018f);
}

} // namespace voxelformat
