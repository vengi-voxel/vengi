/**
 * @file
 */

#include "voxelui/ScriptApi.h"
#include "app/tests/AbstractTest.h"
#include "core/StringUtil.h"
#include "io/Filesystem.h"

namespace voxelui {

class ScriptApiTest : public app::AbstractTest {};

TEST_F(ScriptApiTest, testDetectGeneratorScript) {
	const core::String source = "function arguments()\n"
								"  return {}\n"
								"end\n"
								"function main(node, region, color)\n"
								"end\n";
	EXPECT_EQ("generator", ScriptApi::detectScriptType(source));
}

TEST_F(ScriptApiTest, testDetectBrushScript) {
	const core::String source = "function arguments()\n"
								"  return {}\n"
								"end\n"
								"function generate(node, region, color)\n"
								"end\n";
	EXPECT_EQ("brush", ScriptApi::detectScriptType(source));
}

TEST_F(ScriptApiTest, testDetectSelectionModeScript) {
	const core::String source = "function arguments()\n"
								"  return {}\n"
								"end\n"
								"function select(node, region)\n"
								"end\n";
	EXPECT_EQ("selectionmode", ScriptApi::detectScriptType(source));
}

TEST_F(ScriptApiTest, testDetectUnknownScript) {
	const core::String source = "function foo()\n"
								"end\n";
	EXPECT_EQ("", ScriptApi::detectScriptType(source));
}

TEST_F(ScriptApiTest, testDetectEmptyScript) {
	EXPECT_EQ("", ScriptApi::detectScriptType(""));
}

TEST_F(ScriptApiTest, testScriptTypeToDirGenerator) {
	EXPECT_EQ("scripts", ScriptApi::scriptTypeToDir("generator"));
}

TEST_F(ScriptApiTest, testScriptTypeToDirBrush) {
	EXPECT_EQ("brushes", ScriptApi::scriptTypeToDir("brush"));
}

TEST_F(ScriptApiTest, testScriptTypeToDirSelectionMode) {
	EXPECT_EQ("selectionmodes", ScriptApi::scriptTypeToDir("selectionmode"));
}

TEST_F(ScriptApiTest, testScriptTypeToDirUnknown) {
	EXPECT_EQ("", ScriptApi::scriptTypeToDir("unknown"));
}

TEST_F(ScriptApiTest, testInstallFromFile) {
	const core::String script = "function main(node, region, color)\n"
								"end\n";
	const core::String tmpFile = _testApp->filesystem()->homeWritePath("test_install.lua");
	ASSERT_TRUE(io::Filesystem::sysWrite(tmpFile, script));

	ScriptApi api;
	ASSERT_TRUE(api.install(_testApp->filesystem(), tmpFile));

	const core::String installed = _testApp->filesystem()->homeWritePath(core::string::path("scripts", "test_install.lua"));
	EXPECT_TRUE(io::Filesystem::sysExists(installed));

	// cleanup
	io::Filesystem::sysRemoveFile(installed);
	io::Filesystem::sysRemoveFile(tmpFile);
}

TEST_F(ScriptApiTest, testInstallFromFileURI) {
	const core::String script = "function generate(node, region, color)\n"
								"end\n";
	const core::String tmpFile = _testApp->filesystem()->homeWritePath("test_install_uri.lua");
	ASSERT_TRUE(io::Filesystem::sysWrite(tmpFile, script));

	ScriptApi api;
	const core::String uri = "file://" + tmpFile;
	ASSERT_TRUE(api.install(_testApp->filesystem(), uri));

	const core::String installed = _testApp->filesystem()->homeWritePath(core::string::path("brushes", "test_install_uri.lua"));
	EXPECT_TRUE(io::Filesystem::sysExists(installed));

	// cleanup
	io::Filesystem::sysRemoveFile(installed);
	io::Filesystem::sysRemoveFile(tmpFile);
}

TEST_F(ScriptApiTest, testInstallInvalidExtension) {
	const core::String script = "function main(node, region, color)\n"
								"end\n";
	const core::String tmpFile = _testApp->filesystem()->homeWritePath("test_install.txt");
	ASSERT_TRUE(io::Filesystem::sysWrite(tmpFile, script));

	ScriptApi api;
	EXPECT_FALSE(api.install(_testApp->filesystem(), tmpFile));

	// cleanup
	io::Filesystem::sysRemoveFile(tmpFile);
}

TEST_F(ScriptApiTest, testInstallInvalidScript) {
	const core::String script = "function foo()\nend\n";
	const core::String tmpFile = _testApp->filesystem()->homeWritePath("test_invalid.lua");
	ASSERT_TRUE(io::Filesystem::sysWrite(tmpFile, script));

	ScriptApi api;
	EXPECT_FALSE(api.install(_testApp->filesystem(), tmpFile));

	// cleanup
	io::Filesystem::sysRemoveFile(tmpFile);
}

TEST_F(ScriptApiTest, testUninstallByFilename) {
	const core::String script = "function main(node, region, color)\n"
								"end\n";
	const core::String dir = _testApp->filesystem()->homeWritePath("scripts");
	io::Filesystem::sysCreateDir(dir);
	const core::String path = _testApp->filesystem()->homeWritePath(core::string::path("scripts", "test_uninstall.lua"));
	ASSERT_TRUE(io::Filesystem::sysWrite(path, script));

	ScriptApi api;
	ASSERT_TRUE(api.uninstallByFilename(_testApp->filesystem(), "test_uninstall.lua"));

	EXPECT_FALSE(io::Filesystem::sysExists(path));
}

TEST_F(ScriptApiTest, testUninstallByFilenameNotFound) {
	ScriptApi api;
	EXPECT_FALSE(api.uninstallByFilename(_testApp->filesystem(), "nonexistent.lua"));
}

TEST_F(ScriptApiTest, testSelectionModePriorityOverGenerator) {
	// A script with both select() and main() should be detected as selectionmode
	const core::String source = "function select(node, region)\n"
								"end\n"
								"function main(node, region, color)\n"
								"end\n";
	EXPECT_EQ("selectionmode", ScriptApi::detectScriptType(source));
}

} // namespace voxelui
