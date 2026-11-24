/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "core/ScopedPtr.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryArchive.h"
#include "io/ZipArchive.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/VarUtil.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/private/mesh/ThreeMFFormat.h"
#include "voxelformat/private/qubicle/QBFormat.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class ThreeMFFormatTest : public AbstractFormatTest {};

static int channelDelta(uint8_t a, uint8_t b) {
	const int d = (int)a - (int)b;
	return d < 0 ? -d : d;
}

TEST_F(ThreeMFFormatTest, testLoadSaveRGB) {
	QBFormat src;
	ThreeMFFormat target;
	// palette indices are rebuilt from triangle colors
	const voxel::ValidateFlags flags =
		(voxel::ValidateFlags::All | voxel::ValidateFlags::IgnoreHollow) & ~(voxel::ValidateFlags::Palette);
	testLoadSaveAndLoadSceneGraph("rgb.qb", src, "rgb.3mf", target, flags, 0.014f);
}

TEST_F(ThreeMFFormatTest, testPivotIsBakedOnce) {
	util::ScopedVarChange volumetric(cfg::Voxformat3MFVolumetric, "false");
	palette::Palette palette;
	palette.magicaVoxel();
	voxel::RawVolume original(voxel::Region(0, 0));
	ASSERT_TRUE(original.setVoxel(0, 0, 0, voxel::createVoxel(palette, 1)));

	scenegraph::SceneGraph sceneGraph;
	{
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setName("shifted");
		node.setPalette(palette);
		node.setUnownedVolume(&original);
		node.setPivot(math::Vec3f(0.5f));
		node.transform(0).setLocalTranslation(math::Vec3f(5.0f, 0.0f, 0.0f));
		ASSERT_NE(InvalidNodeId, sceneGraph.emplace(core::move(node)));
	}
	ThreeMFFormat format;
	const io::ArchivePtr archive = helper_archive();
	ASSERT_TRUE(format.save(sceneGraph, "shifted.3mf", archive, testSaveCtx));

	core::ScopedPtr<io::SeekableReadStream> file(archive->readStream("shifted.3mf"));
	ASSERT_TRUE(file);
	const io::ArchivePtr zip = io::openZipArchive(file);
	ASSERT_TRUE(zip);
	core::ScopedPtr<io::SeekableReadStream> model(zip->readStream("3D/3dmodel.model"));
	ASSERT_TRUE(model);
	core::String xml;
	ASSERT_TRUE(model->readString((int)model->size(), xml, false));
	const char *item = strstr(xml.c_str(), "<item ");
	ASSERT_NE(nullptr, item) << xml.c_str();
	const char *itemEnd = strchr(item, '>');
	ASSERT_NE(nullptr, itemEnd);
	const core::String itemTag(item, (size_t)(itemEnd - item));
	EXPECT_EQ(nullptr, strstr(itemTag.c_str(), "transform=")) << itemTag.c_str();

	float minX = 1.0e10f;
	float maxX = -1.0e10f;
	for (const char *cursor = xml.c_str(); (cursor = strstr(cursor, "x=\"")) != nullptr; cursor += 3) {
		float x = 0.0f;
		ASSERT_EQ(1, sscanf(cursor, "x=\"%f\"", &x));
		minX = core_min(minX, x);
		maxX = core_max(maxX, x);
	}
	// translation 5, pivot 0.5 on a 1 voxel cube: vertices sit around 4.5, not at 0 and not at 10
	EXPECT_GT(minX, 3.0f) << xml.c_str();
	EXPECT_LT(maxX, 7.0f) << xml.c_str();
}

TEST_F(ThreeMFFormatTest, testMeshRoundtrip) {
	util::ScopedVarChange volumetric(cfg::Voxformat3MFVolumetric, "false");
	util::ScopedVarChange fill(cfg::VoxformatFillHollow, "false");
	palette::Palette palette;
	palette.magicaVoxel();
	const color::RGBA expected = palette.color(1);
	voxel::RawVolume original(voxel::Region(0, 0));
	ASSERT_TRUE(original.setVoxel(0, 0, 0, voxel::createVoxel(palette, 1)));

	scenegraph::SceneGraph sceneGraph;
	{
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setName("cube");
		node.setPalette(palette);
		node.setUnownedVolume(&original);
		ASSERT_NE(InvalidNodeId, sceneGraph.emplace(core::move(node)));
	}
	ThreeMFFormat format;
	const io::ArchivePtr archive = helper_archive();
	ASSERT_TRUE(format.save(sceneGraph, "cube.3mf", archive, testSaveCtx));

	scenegraph::SceneGraph loaded;
	ASSERT_TRUE(format.load("cube.3mf", archive, loaded, testLoadCtx));
	const scenegraph::SceneGraphNode *node = loaded.firstModelNode();
	ASSERT_NE(nullptr, node);
	ASSERT_NE(nullptr, node->volume());
	int found = 0;
	color::RGBA got(0, 0, 0, 0);
	voxelutil::visitVolume(*node->volume(), [&](int, int, int, const voxel::Voxel &voxel) {
		got = node->palette().color(voxel.getColor());
		++found;
	});
	EXPECT_EQ(1, found);
	EXPECT_EQ(expected, got);
}

TEST_F(ThreeMFFormatTest, testVolumetricKeepsInterior) {
	util::ScopedVarChange volumetric(cfg::Voxformat3MFVolumetric, "true");
	util::ScopedVarChange fill(cfg::VoxformatFillHollow, "true");
	palette::Palette palette;
	palette.magicaVoxel();
	const color::RGBA shell = palette.color(1);
	voxel::RawVolume original(voxel::Region(0, 2));
	for (int z = 0; z < 3; ++z) {
		for (int y = 0; y < 3; ++y) {
			for (int x = 0; x < 3; ++x) {
				if (x == 0 || x == 2 || y == 0 || y == 2 || z == 0 || z == 2) {
					ASSERT_TRUE(original.setVoxel(x, y, z, voxel::createVoxel(palette, 1)));
				}
			}
		}
	}

	scenegraph::SceneGraph sceneGraph;
	{
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setName("shell");
		node.setPalette(palette);
		node.setUnownedVolume(&original);
		ASSERT_NE(InvalidNodeId, sceneGraph.emplace(core::move(node)));
	}
	ThreeMFFormat format;
	const io::ArchivePtr archive = helper_archive();
	ASSERT_TRUE(format.save(sceneGraph, "shell.3mf", archive, testSaveCtx));

	scenegraph::SceneGraph loaded;
	ASSERT_TRUE(format.load("shell.3mf", archive, loaded, testLoadCtx));
	const scenegraph::SceneGraphNode *node = loaded.firstModelNode();
	ASSERT_NE(nullptr, node);
	const voxel::RawVolume *volume = node->volume();
	ASSERT_NE(nullptr, volume);
	EXPECT_EQ(26, voxelutil::countVoxels(*volume));
	EXPECT_TRUE(voxel::isAir(volume->voxel(1, 1, 1).getMaterial()));
	const color::RGBA got = node->palette().color(volume->voxel(0, 0, 0).getColor());
	EXPECT_LE(channelDelta(got.r, shell.r), 3);
	EXPECT_LE(channelDelta(got.g, shell.g), 3);
	EXPECT_LE(channelDelta(got.b, shell.b), 3);
}

TEST_F(ThreeMFFormatTest, testImportUnitCube) {
	util::ScopedVarChange fill(cfg::VoxformatFillHollow, "false");
	const char *model =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<model unit=\"millimeter\" xml:lang=\"en-US\" xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\">\n"
		"<resources><object id=\"1\" type=\"model\" name=\"unit\"><mesh><vertices>\n"
		"<vertex x=\"0\" y=\"0\" z=\"0\"/><vertex x=\"1\" y=\"0\" z=\"0\"/><vertex x=\"1\" y=\"1\" z=\"0\"/><vertex x=\"0\" y=\"1\" z=\"0\"/>\n"
		"<vertex x=\"0\" y=\"0\" z=\"1\"/><vertex x=\"1\" y=\"0\" z=\"1\"/><vertex x=\"1\" y=\"1\" z=\"1\"/><vertex x=\"0\" y=\"1\" z=\"1\"/>\n"
		"</vertices><triangles>\n"
		"<triangle v1=\"0\" v2=\"2\" v3=\"1\"/><triangle v1=\"0\" v2=\"3\" v3=\"2\"/>\n"
		"<triangle v1=\"4\" v2=\"5\" v3=\"6\"/><triangle v1=\"4\" v2=\"6\" v3=\"7\"/>\n"
		"<triangle v1=\"0\" v2=\"1\" v3=\"5\"/><triangle v1=\"0\" v2=\"5\" v3=\"4\"/>\n"
		"<triangle v1=\"3\" v2=\"7\" v3=\"6\"/><triangle v1=\"3\" v2=\"6\" v3=\"2\"/>\n"
		"<triangle v1=\"0\" v2=\"4\" v3=\"7\"/><triangle v1=\"0\" v2=\"7\" v3=\"3\"/>\n"
		"<triangle v1=\"1\" v2=\"2\" v3=\"6\"/><triangle v1=\"1\" v2=\"6\" v3=\"5\"/>\n"
		"</triangles></mesh></object></resources><build><item objectid=\"1\"/></build></model>\n";
	const char *rels =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
		"<Relationship Target=\"/3D/3dmodel.model\" Id=\"rel-1\" Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\"/>\n"
		"</Relationships>\n";
	const char *types =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">\n"
		"<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n"
		"<Default Extension=\"model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>\n"
		"</Types>\n";

	io::BufferedReadWriteStream blob;
	{
		io::ZipArchive zip;
		ASSERT_TRUE(zip.init(&blob));
		{
			core::ScopedPtr<io::SeekableWriteStream> stream(zip.writeStream("[Content_Types].xml"));
			ASSERT_TRUE(stream && stream->writeString(types, false));
		}
		{
			core::ScopedPtr<io::SeekableWriteStream> stream(zip.writeStream("_rels/.rels"));
			ASSERT_TRUE(stream && stream->writeString(rels, false));
		}
		{
			core::ScopedPtr<io::SeekableWriteStream> stream(zip.writeStream("3D/3dmodel.model"));
			ASSERT_TRUE(stream && stream->writeString(model, false));
		}
		zip.shutdown();
	}
	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	ASSERT_TRUE(archive->add("unit.3mf", blob.getBuffer(), (size_t)blob.size()));
	ThreeMFFormat format;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(format.load("unit.3mf", archive, graph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = graph.firstModelNode();
	ASSERT_NE(nullptr, node);
	ASSERT_NE(nullptr, node->volume());
	const int count = voxelutil::countVoxels(*node->volume());
	EXPECT_GE(count, 1);
	const math::Vec3i dim = node->region().getDimensionsInVoxels();
	EXPECT_LE(dim.x, 3);
	EXPECT_LE(dim.y, 3);
	EXPECT_LE(dim.z, 3);
}

} // namespace voxelformat
