/**
 * @file
 */

#include "voxelformat/private/mesh/MeshFormat.h"
#include "core/Color.h"
#include "core/tests/TestColorHelper.h"
#include "image/Image.h"
#include "io/Archive.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "video/ShapeBuilder.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/private/mesh/MeshMaterial.h"
#include "voxelformat/tests/AbstractFormatTest.h"

namespace voxelformat {

class MeshFormatTest : public AbstractFormatTest {};

TEST_F(MeshFormatTest, testSubdivide) {
	MeshTriCollection tinyTris;
	voxelformat::MeshTri meshTri;
	meshTri.vertices[0] = glm::vec3(-8.77272797, -11.43335, -0.154544264);
	meshTri.vertices[1] = glm::vec3(-8.77272701, 11.1000004, -0.154543981);
	meshTri.vertices[2] = glm::vec3(8.77272701, 11.1000004, -0.154543981);
	MeshFormat::subdivideTri(meshTri, tinyTris);
	EXPECT_EQ(1024u, tinyTris.size());
}

TEST_F(MeshFormatTest, testColorAt) {
	const image::ImagePtr &texture = image::loadImage("palette-nippon.png");
	ASSERT_TRUE(texture);
	ASSERT_EQ(256, texture->width());
	ASSERT_EQ(1, texture->height());

	palette::Palette pal;
	pal.nippon();

	voxelformat::MeshTri meshTri;
	meshTri.material = createMaterial(texture);
	for (int i = 0; i < 256; ++i) {
		meshTri.uv[0] = meshTri.uv[1] = meshTri.uv[2] = texture->uv(i, 0);
		const core::RGBA color = meshTri.colorAt(meshTri.centerUV());
		ASSERT_EQ(pal.color(i), color) << "i: " << i << " " << core::Color::print(pal.color(i)) << " vs "
									   << core::Color::print(color);
	}
}

TEST_F(MeshFormatTest, testCalculateAABB) {
	MeshTriCollection tris;
	voxelformat::MeshTri meshTri;

	{
		meshTri.vertices[0] = glm::vec3(0, 0, 0);
		meshTri.vertices[1] = glm::vec3(10, 0, 0);
		meshTri.vertices[2] = glm::vec3(10, 0, 10);
		tris.push_back(meshTri);
	}

	{
		meshTri.vertices[0] = glm::vec3(0, 0, 0);
		meshTri.vertices[1] = glm::vec3(-10, 0, 0);
		meshTri.vertices[2] = glm::vec3(-10, 0, -10);
		tris.push_back(meshTri);
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
	MeshTriCollection tris;
	voxelformat::MeshTri meshTri;

	{
		meshTri.vertices[0] = glm::vec3(0, 0, 0);
		meshTri.vertices[1] = glm::vec3(10, 0, 0);
		meshTri.vertices[2] = glm::vec3(10, 0, 10);
		tris.push_back(meshTri);
	}

	{
		meshTri.vertices[0] = glm::vec3(0, 0, 0);
		meshTri.vertices[1] = glm::vec3(-10, 0, 0);
		meshTri.vertices[2] = glm::vec3(-10, 0, -10);
		tris.push_back(meshTri);
	}

	EXPECT_TRUE(MeshFormat::isVoxelMesh(tris));

	{
		meshTri.vertices[0] = glm::vec3(0, 0, 0);
		meshTri.vertices[1] = glm::vec3(-10, 1, 0);
		meshTri.vertices[2] = glm::vec3(-10, 0, -10);
		tris.push_back(meshTri);
	}

	EXPECT_FALSE(MeshFormat::isVoxelMesh(tris));
}

// TODO: VOXELFORMAT: this test is unstable
TEST_F(MeshFormatTest, testVoxelizeColor) {
	class TestMesh : public MeshFormat {
	public:
		bool saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &, const Meshes &,
						const core::String &, const io::ArchivePtr &, const glm::vec3 &, bool, bool, bool) override {
			return false;
		}
		void voxelize(scenegraph::SceneGraph &sceneGraph, const MeshTriCollection &tris) {
			voxelizeNode("test", sceneGraph, tris);
			sceneGraph.updateTransforms();
		}
	};

	TestMesh mesh;
	MeshTriCollection tris;

	video::ShapeBuilder b;
	scenegraph::SceneGraph sceneGraph;

	voxel::getPalette().nippon();
	const core::RGBA nipponRed = voxel::getPalette().color(37);
	const core::RGBA nipponBlue = voxel::getPalette().color(202);
	const float size = 10.0f;
	b.setPosition({size, 0.0f, size});
	b.setColor(core::Color::fromRGBA(nipponRed));
	b.pyramid({size, size, size});

	const video::ShapeBuilder::Indices &indices = b.getIndices();
	const video::ShapeBuilder::Vertices &vertices = b.getVertices();

	// color of the tip is green
	video::ShapeBuilder::Colors colors = b.getColors();
	const core::RGBA nipponGreen = voxel::getPalette().color(145);
	colors[0] = core::Color::fromRGBA(nipponGreen);
	colors[1] = core::Color::fromRGBA(nipponBlue);

	const int n = (int)indices.size();
	for (int i = 0; i < n; i += 3) {
		voxelformat::MeshTri meshTri;
		for (int j = 0; j < 3; ++j) {
			meshTri.vertices[j] = vertices[indices[i + j]];
			meshTri.color[j] = core::Color::getRGBA(colors[indices[i + j]]);
		}
		tris.push_back(meshTri);
	}
	mesh.voxelize(sceneGraph, tris);
	scenegraph::SceneGraphNode *node = sceneGraph.findNodeByName("test");
	ASSERT_NE(nullptr, node);
	const voxel::RawVolume *v = node->volume();
	const palette::Palette &palette = node->palette();
	EXPECT_COLOR_NEAR(nipponRed, palette.color(v->voxel(0, 0, 0).getColor()), 0.01f);
	EXPECT_COLOR_NEAR(nipponRed, palette.color(v->voxel(size * 2 - 1, 0, size * 2 - 1).getColor()), 0.01f);
	EXPECT_COLOR_NEAR(nipponBlue, palette.color(v->voxel(0, 0, size * 2 - 1).getColor()), 0.06f);
	EXPECT_COLOR_NEAR(nipponRed, palette.color(v->voxel(size * 2 - 1, 0, 0).getColor()), 0.06f);
	EXPECT_COLOR_NEAR(nipponGreen, palette.color(v->voxel(size - 1, size - 1, size - 1).getColor()), 0.01f);
}

} // namespace voxelformat
