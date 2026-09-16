/**
 * @file
 */

#include "voxelformat/FormatConfig.h"
#include "core/ConfigVar.h"
#include "core/Var.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FormatDescription.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/private/binvox/BinVoxFormat.h"
#include "voxelformat/private/magicavoxel/VoxFormat.h"
#include "voxelformat/private/mesh/GLTFFormat.h"
#include "voxelformat/private/mesh/OBJFormat.h"
#include "voxelformat/private/mesh/PLYFormat.h"
#include "voxelformat/private/mesh/STLFormat.h"
#include "voxelformat/private/qubicle/QBTFormat.h"
#include <gtest/gtest.h>
#include <string.h>

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

TEST_F(FormatConfigTest, testFindCVar) {
	const FormatVarMeta *fill = FormatConfig::findVarMeta(cfg::VoxformatFillHollow);
	ASSERT_NE(fill, nullptr);
	EXPECT_NE(0u, fill->flags & FormatCVarFlag_Load);
	EXPECT_EQ(0u, fill->flags & FormatCVarFlag_Save);
	EXPECT_NE(0u, fill->flags & FormatCVarFlag_Mesh);

	const FormatVarMeta *meshMode = FormatConfig::findVarMeta(cfg::VoxformatMeshMode);
	ASSERT_NE(meshMode, nullptr);
	EXPECT_NE(0u, meshMode->flags & FormatCVarFlag_Save);
	EXPECT_NE(0u, meshMode->flags & FormatCVarFlag_Mesh);
	EXPECT_NE(0u, meshMode->flags & FormatCVarFlag_Primary);
	EXPECT_STREQ("Cubes", meshMode->valueTitles[0]);

	EXPECT_EQ(nullptr, FormatConfig::findVarMeta(cfg::VoxelMeshMode));
	EXPECT_EQ(nullptr, FormatConfig::findVarMeta(cfg::VoxelMeshAlloc));
	EXPECT_EQ(nullptr, FormatConfig::findVarMeta("app_version"));
}

TEST_F(FormatConfigTest, testAppliesToMeshSave) {
	const FormatVarMeta *meshMode = FormatConfig::findVarMeta(cfg::VoxformatMeshMode);
	ASSERT_NE(meshMode, nullptr);
	EXPECT_TRUE(FormatConfig::appliesTo(*meshMode, true, GLTFFormat::format()));
	EXPECT_TRUE(FormatConfig::appliesTo(*meshMode, true, OBJFormat::format()));
	EXPECT_FALSE(FormatConfig::appliesTo(*meshMode, false, GLTFFormat::format()));
	EXPECT_FALSE(FormatConfig::appliesTo(*meshMode, true, VoxFormat::format()));
}

TEST_F(FormatConfigTest, testMeshSaveCapabilities) {
	EXPECT_TRUE(FormatConfig::meshSaveSupportsQuads(OBJFormat::format()));
	EXPECT_TRUE(FormatConfig::meshSaveSupportsQuads(PLYFormat::format()));
	EXPECT_FALSE(FormatConfig::meshSaveSupportsQuads(GLTFFormat::format()));
	EXPECT_FALSE(FormatConfig::meshSaveSupportsQuads(STLFormat::format()));

	EXPECT_TRUE(FormatConfig::meshSaveSupportsColor(OBJFormat::format()));
	EXPECT_TRUE(FormatConfig::meshSaveSupportsColor(GLTFFormat::format()));
	EXPECT_FALSE(FormatConfig::meshSaveSupportsColor(STLFormat::format()));
	EXPECT_EQ(FormatConfig::meshSaveSupportsColor(STLFormat::format()),
			  FormatConfig::meshSaveSupportsTexCoords(STLFormat::format()));
	EXPECT_TRUE(FormatConfig::meshSaveSupportsTexCoords(OBJFormat::format()));

	const FormatVarMeta *quads = FormatConfig::findVarMeta(cfg::VoxformatQuads);
	ASSERT_NE(quads, nullptr);
	EXPECT_TRUE(FormatConfig::appliesTo(*quads, true, OBJFormat::format()));
	EXPECT_TRUE(FormatConfig::appliesTo(*quads, true, PLYFormat::format()));
	EXPECT_FALSE(FormatConfig::appliesTo(*quads, true, GLTFFormat::format()));
	EXPECT_FALSE(FormatConfig::appliesTo(*quads, true, STLFormat::format()));

	const FormatVarMeta *withColor = FormatConfig::findVarMeta(cfg::VoxformatWithColor);
	ASSERT_NE(withColor, nullptr);
	EXPECT_TRUE(FormatConfig::appliesTo(*withColor, true, GLTFFormat::format()));
	EXPECT_FALSE(FormatConfig::appliesTo(*withColor, true, STLFormat::format()));
}

TEST_F(FormatConfigTest, testAppliesToFormatSpecific) {
	const FormatVarMeta *binvox = FormatConfig::findVarMeta(cfg::VoxformatBinvoxVersion);
	ASSERT_NE(binvox, nullptr);
	EXPECT_EQ(&BinVoxFormat::format(), binvox->formats[0]);
	EXPECT_TRUE(FormatConfig::appliesTo(*binvox, true, BinVoxFormat::format()));
	EXPECT_FALSE(FormatConfig::appliesTo(*binvox, true, VoxFormat::format()));
	EXPECT_FALSE(FormatConfig::appliesTo(*binvox, false, BinVoxFormat::format()));

	const FormatVarMeta *qbtLoad = FormatConfig::findVarMeta(cfg::VoxformatQBTMergeCompounds);
	ASSERT_NE(qbtLoad, nullptr);
	EXPECT_TRUE(FormatConfig::appliesTo(*qbtLoad, false, QBTFormat::format()));
	EXPECT_FALSE(FormatConfig::appliesTo(*qbtLoad, true, QBTFormat::format()));

	const FormatVarMeta *gltf = FormatConfig::findVarMeta(cfg::VoxformatGLTF_KHR_materials_specular);
	ASSERT_NE(gltf, nullptr);
	EXPECT_TRUE(FormatConfig::appliesTo(*gltf, true, GLTFFormat::format()));
	EXPECT_FALSE(FormatConfig::appliesTo(*gltf, true, OBJFormat::format()));
}

TEST_F(FormatConfigTest, testAppliesToGenericSave) {
	const FormatVarMeta *merge = FormatConfig::findVarMeta(cfg::VoxformatMerge);
	ASSERT_NE(merge, nullptr);
	EXPECT_TRUE(FormatConfig::appliesTo(*merge, true, VoxFormat::format()));
	EXPECT_TRUE(FormatConfig::appliesTo(*merge, true, GLTFFormat::format()));
	EXPECT_FALSE(FormatConfig::appliesTo(*merge, false, VoxFormat::format()));
}

TEST_F(FormatConfigTest, testFormatNamesExist) {
	const FormatVarMeta *cvars = FormatConfig::varsMeta();
	ASSERT_NE(cvars, nullptr);
	const int count = FormatConfig::cvarCount();
	ASSERT_GT(count, 0);
	for (int i = 0; i < count; ++i) {
		for (int f = 0; f < FormatVarMeta::MaxFormats; ++f) {
			const io::FormatDescription *format = cvars[i].formats[f];
			if (format == nullptr) {
				break;
			}
			bool found = false;
			for (const io::FormatDescription *desc = voxelLoad(); desc->valid(); ++desc) {
				if (*desc == *format) {
					found = true;
					break;
				}
			}
			EXPECT_TRUE(found) << "Unknown format '" << format->name.c_str() << "' on cvar " << cvars[i].name;
		}
		if (cvars[i].flags & (FormatCVarFlag_Load | FormatCVarFlag_Save)) {
			ASSERT_NE(core::findVar(cvars[i].name), nullptr) << "Unregistered cvar " << cvars[i].name;
		}
	}
}

TEST_F(FormatConfigTest, testWriteConfigJsonExtra) {
	io::BufferedReadWriteStream stream;
	FormatConfig::writeConfigJson(stream, core::getVar(cfg::VoxformatFillHollow));
	ASSERT_TRUE(stream.writeUInt8(0));
	const char *json = (const char *)stream.getBuffer();
	ASSERT_NE(json, nullptr);
	EXPECT_NE(strstr(json, "\"load\": true"), nullptr);
	EXPECT_NE(strstr(json, "\"save\": false"), nullptr);
	EXPECT_NE(strstr(json, "\"mesh\": true"), nullptr);
	EXPECT_NE(strstr(json, "\"primary\": true"), nullptr);
	EXPECT_NE(strstr(json, "\"order\":"), nullptr);

	io::BufferedReadWriteStream meshStream;
	FormatConfig::writeConfigJson(meshStream, core::getVar(cfg::VoxformatMeshMode));
	ASSERT_TRUE(meshStream.writeUInt8(0));
	const char *meshJson = (const char *)meshStream.getBuffer();
	ASSERT_NE(meshJson, nullptr);
	EXPECT_NE(strstr(meshJson, "\"save\": true"), nullptr);
	EXPECT_NE(strstr(meshJson, "\"primary\": true"), nullptr);
	EXPECT_NE(strstr(meshJson, "\"value_titles\": ["), nullptr);
	EXPECT_NE(strstr(meshJson, "Cubes"), nullptr);

	io::BufferedReadWriteStream binvoxStream;
	FormatConfig::writeConfigJson(binvoxStream, core::getVar(cfg::VoxformatBinvoxVersion));
	ASSERT_TRUE(binvoxStream.writeUInt8(0));
	const char *binvoxJson = (const char *)binvoxStream.getBuffer();
	ASSERT_NE(binvoxJson, nullptr);
	EXPECT_NE(strstr(binvoxJson, "\"formats\": [\"BinVox\"]"), nullptr);
	EXPECT_NE(strstr(binvoxJson, "\"primary\": false"), nullptr);
	EXPECT_NE(strstr(binvoxJson, "Binvox 1 (white)"), nullptr);

	io::BufferedReadWriteStream quadsStream;
	FormatConfig::writeConfigJson(quadsStream, core::getVar(cfg::VoxformatQuads));
	ASSERT_TRUE(quadsStream.writeUInt8(0));
	const char *quadsJson = (const char *)quadsStream.getBuffer();
	ASSERT_NE(quadsJson, nullptr);
	EXPECT_NE(strstr(quadsJson, "\"formats\": ["), nullptr);
	EXPECT_NE(strstr(quadsJson, "Wavefront Object"), nullptr);
	EXPECT_NE(strstr(quadsJson, "Polygon File Format"), nullptr);
	EXPECT_EQ(strstr(quadsJson, "GL Transmission Format"), nullptr);
	EXPECT_EQ(strstr(quadsJson, "Standard Triangle Language"), nullptr);

	io::BufferedReadWriteStream colorStream;
	FormatConfig::writeConfigJson(colorStream, core::getVar(cfg::VoxformatWithColor));
	ASSERT_TRUE(colorStream.writeUInt8(0));
	const char *colorJson = (const char *)colorStream.getBuffer();
	ASSERT_NE(colorJson, nullptr);
	EXPECT_NE(strstr(colorJson, "GL Transmission Format"), nullptr);
	EXPECT_EQ(strstr(colorJson, "Standard Triangle Language"), nullptr);
}

TEST_F(FormatConfigTest, testImageSaveTypeAllowsThumbnail) {
	const core::VarPtr &saveType = core::getVar(cfg::VoxformatImageSaveType);
	ASSERT_TRUE(saveType->hasMinMax());
	EXPECT_EQ(0, saveType->intMinValue());
	EXPECT_EQ(3, saveType->intMaxValue());
}

} // namespace voxelformat
