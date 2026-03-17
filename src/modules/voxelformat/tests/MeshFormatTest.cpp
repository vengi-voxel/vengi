/**
 * @file
 */

#include "voxelformat/private/mesh/MeshFormat.h"
#include "color/Color.h"
#include "core/ConfigVar.h"
#include "core/tests/TestColorHelper.h"
#include "image/Image.h"
#include "io/Archive.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/VarUtil.h"
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
	meshTri.setVertices(glm::vec3(-8.77272797, -11.43335, -0.154544264),
						glm::vec3(-8.77272701, 11.1000004, -0.154543981),
						glm::vec3(8.77272701, 11.1000004, -0.154543981));
	int depth = 0;
	MeshFormat::subdivideTri(meshTri, tinyTris, depth);
	EXPECT_EQ(1024u, tinyTris.size());
}

TEST_F(MeshFormatTest, testColorAt) {
	const image::ImagePtr &texture = image::loadImage("palette-nippon.png");
	ASSERT_TRUE(texture);
	ASSERT_EQ(256, texture->width());
	ASSERT_EQ(1, texture->height());

	palette::Palette pal;
	pal.nippon();

	MeshMaterialArray meshMaterialArray;
	voxelformat::MeshTri meshTri;
	meshMaterialArray.emplace_back(createMaterial(texture));
	meshTri.materialIdx = meshMaterialArray.size() - 1;
	for (int i = 0; i < 256; ++i) {
		const glm::vec2 uv = texture->uv(i, 0);
		meshTri.setUVs(uv, uv, uv);
		const color::RGBA color = colorAt(meshTri, meshMaterialArray, meshTri.centerUV());
		ASSERT_EQ(pal.color(i), color) << "i: " << i << " " << color::print(pal.color(i)) << " vs "
									   << color::print(color);
	}
}

TEST_F(MeshFormatTest, testCalculateAABB) {
	MeshTriCollection tris;
	voxelformat::MeshTri meshTri;

	{
		meshTri.setVertices(glm::vec3(0, 0, 0), glm::vec3(10, 0, 0), glm::vec3(10, 0, 10));
		tris.push_back(meshTri);
	}

	{
		meshTri.setVertices(glm::vec3(0, 0, 0), glm::vec3(-10, 0, 0), glm::vec3(-10, 0, -10));
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
		meshTri.setVertices(glm::vec3(0, 0, 0), glm::vec3(10, 0, 0), glm::vec3(10, 0, 10));
		tris.push_back(meshTri);
	}

	{
		meshTri.setVertices(glm::vec3(0, 0, 0), glm::vec3(-10, 0, 0), glm::vec3(-10, 0, -10));
		tris.push_back(meshTri);
	}

	EXPECT_TRUE(MeshFormat::isVoxelMesh(tris));

	{
		meshTri.setVertices(glm::vec3(0, 0, 0), glm::vec3(-10, 1, 0), glm::vec3(-10, 0, -10));
		tris.push_back(meshTri);
	}

	EXPECT_FALSE(MeshFormat::isVoxelMesh(tris));
}

TEST_F(MeshFormatTest, testVoxelizeColor) {
	class TestMesh : public MeshFormat {
	public:
		bool saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &, const ChunkMeshes &,
						const core::String &, const io::ArchivePtr &, const glm::vec3 &, bool, bool, bool) override {
			return false;
		}
		void voxelize(scenegraph::SceneGraph &sceneGraph, Mesh &&mesh) {
			voxelizeMesh("test", sceneGraph, core::move(mesh));
			sceneGraph.updateTransforms();
		}
	};

	TestMesh testMesh;
	Mesh mesh;
	video::ShapeBuilder b;
	scenegraph::SceneGraph sceneGraph;

	palette::Palette pal;
	pal.nippon();
	const color::RGBA nipponRed = pal.color(37);
	const color::RGBA nipponBlue = pal.color(202);
	const float size = 10.0f;
	b.setPosition({size, 0.0f, size});
	b.setColor(color::fromRGBA(nipponRed));
	b.pyramid({size, size, size});

	const video::ShapeBuilder::Indices &indices = b.getIndices();
	const video::ShapeBuilder::Vertices &vertices = b.getVertices();

	// color of the tip is green
	video::ShapeBuilder::Colors colors = b.getColors();
	const color::RGBA nipponGreen = pal.color(145);
	colors[0] = color::fromRGBA(nipponGreen);
	colors[1] = color::fromRGBA(nipponBlue);

	const int n = (int)indices.size();
	for (int i = 0; i < n; i += 3) {
		voxelformat::MeshTri meshTri;
		meshTri.setVertices(vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]]);
		meshTri.setColor(color::getRGBA(colors[indices[i]]),
						 color::getRGBA(colors[indices[i + 1]]),
						 color::getRGBA(colors[indices[i + 2]]));
		mesh.addTriangle(meshTri);
	}
	testMesh.voxelize(sceneGraph, core::move(mesh));
	scenegraph::SceneGraphNode *node = sceneGraph.findNodeByName("test");
	ASSERT_NE(nullptr, node);
	const voxel::RawVolume *v = node->volume();
	const palette::Palette &nodePal = node->palette();
	EXPECT_COLOR_NEAR(nipponRed, nodePal.color(v->voxel(0, 0, 0).getColor()), 0.06f);
	EXPECT_COLOR_NEAR(nipponRed, nodePal.color(v->voxel(size * 2 - 1, 0, size * 2 - 1).getColor()), 0.06f);
	EXPECT_COLOR_NEAR(nipponBlue, nodePal.color(v->voxel(0, 0, size * 2 - 1).getColor()), 0.06f);
	EXPECT_COLOR_NEAR(nipponRed, nodePal.color(v->voxel(size * 2 - 1, 0, 0).getColor()), 0.06f);
	EXPECT_COLOR_NEAR(nipponGreen, nodePal.color(v->voxel(size - 1, size - 1, size - 1).getColor()), 0.06f);
}

TEST_F(MeshFormatTest, testSaveAsPointCloudUsesVoxelCenters) {
	class TestMesh : public MeshFormat {
	public:
		mutable PointCloud savedPointCloud;

		bool saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &, const ChunkMeshes &,
						const core::String &, const io::ArchivePtr &, const glm::vec3 &, bool, bool, bool) override {
			return false;
		}

		bool savePointCloud(const scenegraph::SceneGraph &, const PointCloud &pointCloud, const core::String &,
							const io::ArchivePtr &, const glm::vec3 &, bool) const override {
			savedPointCloud = pointCloud;
			return true;
		}
	};

	TestMesh testMesh;
	scenegraph::SceneGraph sceneGraph;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	voxel::RawVolume *volume = new voxel::RawVolume(voxel::Region(glm::ivec3(0), glm::ivec3(1, 0, 0)));
	palette::Palette palette;
	palette.nippon();
	const color::RGBA nipponRed = palette.color(37);
	volume->setVoxel(0, 0, 0, voxel::createVoxel(palette, 37));
	volume->setVoxel(1, 0, 0, voxel::createVoxel(palette, 37));
	node.setVolume(volume, true);
	node.setPalette(palette);
	sceneGraph.emplace(core::move(node));

	util::ScopedVarChange pointCloudVarChange(cfg::VoxformatPointCloud, "true");
	ASSERT_TRUE(testMesh.saveGroups(sceneGraph, "test.ply", nullptr, {}));

	ASSERT_EQ(2u, testMesh.savedPointCloud.size());
	EXPECT_EQ(glm::vec3(0.5f, 0.5f, 0.5f), testMesh.savedPointCloud[0].position);
	EXPECT_EQ(glm::vec3(1.5f, 0.5f, 0.5f), testMesh.savedPointCloud[1].position);
	EXPECT_EQ(nipponRed, testMesh.savedPointCloud[0].color);
	EXPECT_EQ(nipponRed, testMesh.savedPointCloud[1].color);
}

} // namespace voxelformat
