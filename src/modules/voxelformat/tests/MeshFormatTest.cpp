/**
 * @file
 */

#include "color/ColorUtil.h"
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
#include "voxel/SurfaceExtractor.h"
#include "voxel/Voxel.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/private/mesh/MeshMaterial.h"
#include "voxelformat/tests/AbstractFormatTest.h"
#include "voxelutil/VolumeVisitor.h"
#include <limits>
#include <glm/gtc/quaternion.hpp>

namespace voxelformat {

class MeshFormatTest : public AbstractFormatTest {};

TEST_F(MeshFormatTest, testExportIgnoresVoxelNormals) {
	class TestMesh : public MeshFormat {
		bool saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &, const ChunkMeshes &meshes,
						const core::String &, const io::ArchivePtr &, const glm::vec3 &, bool, bool, bool) override {
			vertices = indices = normals = 0;
			for (const ChunkMeshExt &entry : meshes) {
				for (const voxel::Mesh &mesh : entry.mesh->mesh) {
					vertices += mesh.getNoOfVertices();
					indices += mesh.getNoOfIndices();
					normals += mesh.getNormalVector().size();
					for (const voxel::VoxelVertex &v : mesh.getVertexVector()) {
						EXPECT_EQ(NO_NORMAL, v.normalIndex);
					}
				}
			}
			return true;
		}
	public:
		size_t vertices = 0;
		size_t indices = 0;
		size_t normals = 0;
	};
	util::ScopedVarChange optimize(cfg::VoxformatOptimize, "false");
	util::ScopedVarChange merge(cfg::VoxformatMergequads, "true");
	util::ScopedVarChange reuse(cfg::VoxformatReusevertices, "true");
	scenegraph::SceneGraph graph;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.createVolume(voxel::Region(0, 0, 0, 5, 1, 1));
	palette::Palette pal;
	pal.nippon();
	node.setPalette(pal);
	voxel::RawVolume *volume = node.volume();
	graph.emplace(core::move(node));
	graph.updateTransforms();
	const voxel::SurfaceExtractionType types[] = {voxel::SurfaceExtractionType::Cubic, voxel::SurfaceExtractionType::Binary};
	for (voxel::SurfaceExtractionType type : types) {
		util::ScopedVarChange meshMode(cfg::VoxformatMeshMode, (int)type);
		for (int withNormals = 0; withNormals <= 1; ++withNormals) {
			util::ScopedVarChange exportNormals(cfg::VoxformatWithNormals, withNormals);
			size_t vertices = 0;
			size_t indices = 0;
			for (int voxelNormals = 0; voxelNormals <= 1; ++voxelNormals) {
				for (int x = 0; x <= 5; ++x) {
					for (int y = 0; y <= 1; ++y) {
						for (int z = 0; z <= 1; ++z) {
							volume->setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 42,
								voxelNormals ? 1 + x % 2 : NO_NORMAL));
						}
					}
				}
				TestMesh format;
				ASSERT_TRUE(format.saveGroups(graph, "mesh", {}, testSaveCtx));
				ASSERT_GT(format.vertices, 0u);
				EXPECT_EQ(withNormals ? format.vertices : 0u, format.normals);
				if (voxelNormals == 0) {
					vertices = format.vertices;
					indices = format.indices;
				} else {
					EXPECT_EQ(vertices, format.vertices);
					EXPECT_EQ(indices, format.indices);
					EXPECT_EQ(2, volume->voxel(1, 0, 0).getNormal());
				}
			}
		}
	}
}

TEST_F(MeshFormatTest, testSubdivide) {
	MeshTriCollection tinyTris;
	voxelformat::MeshTri meshTri;
	meshTri.setVertices(glm::vec3(-8.77272797, -11.43335, -0.154544264),
						glm::vec3(-8.77272701, 11.1000004, -0.154543981),
						glm::vec3(8.77272701, 11.1000004, -0.154543981));
	MeshFormat::subdivideTri(meshTri, tinyTris, 0);
	EXPECT_EQ(1024u, tinyTris.size());
}

TEST_F(MeshFormatTest, testSubdivideRejectsTooLarge) {
	// Triangles whose AABB exceeds 2^(MaxSubdivideDepth - depth) cannot reach size <= 1
	// within the depth budget; early-out instead of exploding toward 4^depth leaves.
	MeshTriCollection tinyTris;
	voxelformat::MeshTri meshTri;
	meshTri.setVertices(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0e6f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0e6f, 0.0f));
	MeshFormat::subdivideTri(meshTri, tinyTris, 0);
	EXPECT_FALSE(tinyTris.empty());
	EXPECT_LT(tinyTris.size(), 16u);
}

TEST_F(MeshFormatTest, testSubdivideSkipsNonFinite) {
	MeshTriCollection tinyTris;
	voxelformat::MeshTri meshTri;
	const float inf = std::numeric_limits<float>::infinity();
	meshTri.setVertices(glm::vec3(inf, 0.0f, 0.0f), glm::vec3(0.0f, inf, 0.0f), glm::vec3(0.0f, 0.0f, inf));
	EXPECT_FALSE(MeshFormat::subdivideTri(meshTri, tinyTris, 0));
	EXPECT_TRUE(tinyTris.empty());
}

TEST_F(MeshFormatTest, testVoxelizeSkipsSubdivisionForVoxelSizedTris) {
	// Non-axis-aligned tris that already fit in one voxel cell should take the high-quality
	// path without the parallel subdivide/copy step.
	class TestMesh : public MeshFormat {
	public:
		bool saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &, const ChunkMeshes &,
						const core::String &, const io::ArchivePtr &, const glm::vec3 &, bool, bool, bool) override {
			return false;
		}
		int voxelize(scenegraph::SceneGraph &sceneGraph, Mesh &&mesh) {
			return importMesh("tiny", sceneGraph, core::move(mesh), 0, false);
		}
	};

	util::ScopedVarChange voxelizeMode(cfg::VoxformatVoxelizeMode, "0"); // HighQuality
	util::ScopedVarChange createPalette(cfg::VoxelCreatePalette, "true");
	util::ScopedVarChange fillHollow(cfg::VoxformatFillHollow, "false");

	Mesh mesh;
	voxelformat::MeshTri meshTri;
	meshTri.setColor(color::RGBA(255, 0, 0), color::RGBA(255, 0, 0), color::RGBA(255, 0, 0));
	meshTri.setVertices(glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(0.7f, 0.2f, 0.15f), glm::vec3(0.2f, 0.65f, 0.25f));
	const glm::vec3 size = meshTri.maxs() - meshTri.mins();
	ASSERT_FALSE(glm::any(glm::greaterThan(size, glm::vec3(1.0f))));
	mesh.addTriangle(meshTri);

	MeshTriCollection checkTris;
	checkTris.push_back(meshTri);
	ASSERT_FALSE(MeshFormat::isVoxelMesh(checkTris));

	TestMesh testMesh;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_NE(InvalidNodeId, testMesh.voxelize(sceneGraph, core::move(mesh)));
	const scenegraph::SceneGraphNode *node = sceneGraph.findNodeByName("tiny");
	ASSERT_NE(nullptr, node);
	ASSERT_NE(nullptr, node->volume());
	EXPECT_GT(voxelutil::countVoxels(*node->volume()), 0);
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

TEST_F(MeshFormatTest, testSceneVoxelSize) {
	class TestMesh : public MeshFormat {
		bool saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &, const ChunkMeshes &,
						const core::String &, const io::ArchivePtr &, const glm::vec3 &, bool, bool, bool) override {
			return false;
		}
		bool voxelizeGroups(const core::String &, const io::ArchivePtr &, scenegraph::SceneGraph &graph,
							const LoadContext &ctx) override {
			const glm::vec3 scale = getInputScale(glm::vec3(0), glm::vec3(14, 4, 0));
			for (int i = 0; i < 2; ++i) {
				Mesh mesh;
				MeshTri tri;
				const float size = i == 0 ? 2.0f : 4.0f;
				tri.setVertices({0, 0, 0}, {size, 0, 0}, {0, size, 0});
				tri.setColor(color::RGBA(255, 0, 0));
				mesh.addTriangle(tri);
				const int id = importMesh(i == 0 ? "small" : "large", graph, core::move(mesh), 0, false, ctx.progress);
				if (id == InvalidNodeId) {
					return false;
				}
				graph.node(id).setTranslation(glm::vec3(i * 10, 0, 0) * scale);
			}
			return true;
		}
	};
	util::ScopedVarChange voxelMode(cfg::VoxformatVoxelizeMode, "1");
	util::ScopedVarChange createPalette(cfg::VoxelCreatePalette, "true");
	util::ScopedVarChange fillHollow(cfg::VoxformatFillHollow, "false");
	TestMesh format;
	const int sizes[] = {28, 14, 0};
	for (int size : sizes) {
		util::ScopedVarChange voxelSize(cfg::VoxformatVoxelSize, size);
		scenegraph::SceneGraph graph;
		ASSERT_TRUE(format.loadGroups("scene", {}, graph, testLoadCtx));
		const scenegraph::SceneGraphNode *small = graph.findNodeByName("small");
		const scenegraph::SceneGraphNode *large = graph.findNodeByName("large");
		ASSERT_NE(nullptr, small);
		ASSERT_NE(nullptr, large);
		const float scale = size == 28 ? 2.0f : 1.0f;
		EXPECT_LE(small->region().getDimensionsInVoxels().x, (int)(2 * scale + 1));
		EXPECT_LE(large->region().getDimensionsInVoxels().x, (int)(4 * scale + 1));
		EXPECT_GE(small->region().getDimensionsInVoxels().x, (int)(2 * scale));
		EXPECT_GE(large->region().getDimensionsInVoxels().x, (int)(4 * scale));
		EXPECT_FLOAT_EQ(10 * scale, large->transform(0).worldTranslation().x);
	}
}

TEST_F(MeshFormatTest, testSceneVoxelSizeTransformedBounds) {
	class TestMesh : public MeshFormat {
		bool saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &, const ChunkMeshes &,
						const core::String &, const io::ArchivePtr &, const glm::vec3 &, bool, bool, bool) override {
			return false;
		}
		bool voxelizeGroups(const core::String &, const io::ArchivePtr &, scenegraph::SceneGraph &graph,
							const LoadContext &ctx) override {
			Mesh mesh;
			MeshTri tri;
			tri.setVertices({0, 1, 0}, {1, 0, 0}, {2, 2, 0});
			tri.setColor(color::RGBA(255, 0, 0));
			mesh.addTriangle(tri);
			inputScale = getInputScale(glm::vec3(0), glm::vec3(2, 2, 0));
			const int id = importMesh("triangle", graph, core::move(mesh), 0, false, ctx.progress);
			if (id == InvalidNodeId) {
				return false;
			}
			graph.node(id).transform(0).setLocalOrientation(rotation);
			graph.node(id).transform(0).setLocalScale(scale);
			graph.node(id).setTranslation(glm::vec3(10, -5, 0) * inputScale);
			return true;
		}
	public:
		glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
		glm::vec3 scale{1.0f};
		glm::vec3 inputScale{1.0f};
	};
	util::ScopedVarChange voxelSize(cfg::VoxformatVoxelSize, "20");
	util::ScopedVarChange voxelMode(cfg::VoxformatVoxelizeMode, "1");
	util::ScopedVarChange createPalette(cfg::VoxelCreatePalette, "true");
	util::ScopedVarChange fillHollow(cfg::VoxformatFillHollow, "false");
	TestMesh format;
	{
		scenegraph::SceneGraph graph;
		ASSERT_TRUE(format.loadGroups("scene", {}, graph, testLoadCtx));
		EXPECT_NEAR(format.inputScale.x, 10.0f, 0.0001f);
	}
	{
		// The local AABB has a larger rotated extent than this triangle itself.
		format.rotation = glm::angleAxis(glm::radians(45.0f), glm::vec3(0, 0, 1));
		scenegraph::SceneGraph graph;
		ASSERT_TRUE(format.loadGroups("scene", {}, graph, testLoadCtx));
		EXPECT_NEAR(format.inputScale.x, 20.0f / (3.0f / glm::sqrt(2.0f)), 0.0001f);
	}
	{
		format.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		format.scale = glm::vec3(-2, 3, 1);
		scenegraph::SceneGraph graph;
		ASSERT_TRUE(format.loadGroups("scene", {}, graph, testLoadCtx));
		EXPECT_NEAR(format.inputScale.x, 20.0f / 6.0f, 0.0001f);
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
	for (int mode = 0; mode <= 1; ++mode) {
		for (int createPalette = mode == 0 ? 1 : 0; createPalette <= 1; ++createPalette) {
			SCOPED_TRACE(mode);
			SCOPED_TRACE(createPalette);
			util::ScopedVarChange modeVar(cfg::VoxformatVoxelizeMode, mode == 0 ? "0" : "1");
			util::ScopedVarChange paletteVar(cfg::VoxelCreatePalette, createPalette ? "true" : "false");
			class TestMesh : public MeshFormat {
			public:
				bool saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &, const ChunkMeshes &,
								const core::String &, const io::ArchivePtr &, const glm::vec3 &, bool, bool, bool) override {
					return false;
				}
				void voxelize(scenegraph::SceneGraph &sceneGraph, Mesh &&mesh) {
					importMesh("test", sceneGraph, core::move(mesh));
					sceneGraph.updateTransforms();
				}
			};

			TestMesh testMesh;
			Mesh mesh;
			video::ShapeBuilder b;
			scenegraph::SceneGraph sceneGraph;

			palette::Palette pal;
			if (createPalette) {
				pal.nippon();
			} else {
				pal = voxel::getPalette();
			}
			ASSERT_GT(pal.colorCount(), 202);
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
	}
}

TEST_F(MeshFormatTest, testVoxelizeChunked) {
	class TestMesh : public MeshFormat {
	public:
		bool saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &, const ChunkMeshes &,
						const core::String &, const io::ArchivePtr &, const glm::vec3 &, bool, bool, bool) override {
			return false;
		}
		void voxelize(scenegraph::SceneGraph &sceneGraph, Mesh &&mesh) {
			importMesh("test", sceneGraph, core::move(mesh));
			sceneGraph.updateTransforms();
		}
	};

	// Enable chunked mode with small chunk size to force multiple chunks
	util::ScopedVarChange chunkedVar(cfg::VoxformatVoxelizeChunked, "true");
	util::ScopedVarChange chunkSizeVar(cfg::VoxformatVoxelizeChunkSize, "64");
	util::ScopedVarChange createPaletteVar(cfg::VoxelCreatePalette, "true");

	TestMesh testMesh;
	Mesh mesh;
	scenegraph::SceneGraph sceneGraph;

	palette::Palette pal;
	pal.nippon();
	const color::RGBA nipponRed = pal.color(37);

	// Create two large triangles forming a flat quad > 256 voxels wide to trigger chunked path.
	// The quad spans from (0,0,0) to (300,0,300) on the XZ plane.
	const float extent = 300.0f;
	{
		voxelformat::MeshTri meshTri;
		meshTri.setVertices(glm::vec3(0, 0, 0), glm::vec3(extent, 0, 0), glm::vec3(extent, 0, extent));
		meshTri.setColor(nipponRed, nipponRed, nipponRed);
		mesh.addTriangle(meshTri);
	}
	{
		voxelformat::MeshTri meshTri;
		meshTri.setVertices(glm::vec3(0, 0, 0), glm::vec3(extent, 0, extent), glm::vec3(0, 0, extent));
		meshTri.setColor(nipponRed, nipponRed, nipponRed);
		mesh.addTriangle(meshTri);
	}

	testMesh.voxelize(sceneGraph, core::move(mesh));

	// Should have created a group node containing multiple chunk children
	const scenegraph::SceneGraphNode *groupNode = sceneGraph.findNodeByName("test");
	ASSERT_NE(nullptr, groupNode);
	ASSERT_EQ(scenegraph::SceneGraphNodeType::Group, groupNode->type());

	// Count model children - with 300 voxels and 64-chunk size we expect multiple chunks
	int modelCount = 0;
	int totalVoxels = 0;
	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		++modelCount;
		const voxel::RawVolume *volume = node.volume();
		ASSERT_NE(nullptr, volume);
		// Verify voxels have the expected color
		const palette::Palette &nodePal = node.palette();
		voxelutil::visitVolume(
			*volume,
			[&totalVoxels, &nodePal, &nipponRed](int, int, int, const voxel::Voxel &voxel) {
				++totalVoxels;
				EXPECT_COLOR_NEAR(nipponRed, nodePal.color(voxel.getColor()), 0.1f);
			},
			voxelutil::VisitAll());
	}
	EXPECT_GT(modelCount, 1) << "Chunked voxelization should produce multiple chunk nodes";
	EXPECT_GT(totalVoxels, 0) << "Chunks should contain voxels";
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
	node.setVolume(volume);
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

TEST_F(MeshFormatTest, testSaveWithNodeIdHole) {
	scenegraph::SceneGraph sceneGraph;
	{
		scenegraph::SceneGraphNode empty(scenegraph::SceneGraphNodeType::Model);
		empty.createVolume(voxel::Region(0, 1));
		empty.setName("empty");
		ASSERT_NE(InvalidNodeId, sceneGraph.emplace(core::move(empty)));
	}
	{
		scenegraph::SceneGraphNode filled(scenegraph::SceneGraphNodeType::Model);
		voxel::RawVolume *volume = new voxel::RawVolume(voxel::Region(0, 3));
		volume->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		filled.setVolume(volume);
		filled.setName("filled");
		ASSERT_NE(InvalidNodeId, sceneGraph.emplace(core::move(filled)));
	}
	ASSERT_TRUE(sceneGraph.removeNode(1, false));
	ASSERT_FALSE(sceneGraph.hasNode(1));
	ASSERT_TRUE(sceneGraph.hasNode(2));
	ASSERT_TRUE(helper_saveSceneGraph(sceneGraph, "test-nodeid-hole.obj"));
}

} // namespace voxelformat
