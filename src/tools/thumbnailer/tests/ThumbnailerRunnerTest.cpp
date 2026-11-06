/**
 * @file
 */

#include "ThumbnailerRunner.h"
#include "core/StringUtil.h"
#include "io/Filesystem.h"
#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

class ThumbnailerRunnerTest : public testing::Test {};

TEST_F(ThumbnailerRunnerTest, testFindThumbnailerSameDirectory) {
	char templatePath[] = "/tmp/vengi-thumbfind.XXXXXX";
	ASSERT_NE(mkdtemp(templatePath), nullptr);
	const core::String dir(templatePath);
	const core::String modulePath = core::string::path(dir, "plugin.bin");
	const core::String thumbnailerPath = core::string::path(dir, "vengi-thumbnailer");

	FILE *moduleFile = fopen(modulePath.c_str(), "w");
	ASSERT_NE(moduleFile, nullptr);
	fputs("module", moduleFile);
	fclose(moduleFile);

	FILE *thumbFile = fopen(thumbnailerPath.c_str(), "w");
	ASSERT_NE(thumbFile, nullptr);
	fputs("#!/bin/sh\nexit 0\n", thumbFile);
	fclose(thumbFile);
	ASSERT_EQ(chmod(thumbnailerPath.c_str(), 0755), 0);

	const core::String found = thumbnailer::findThumbnailerExecutable(modulePath);
	EXPECT_EQ(thumbnailerPath, found);

	io::Filesystem::sysRemoveDir(dir, true);
}

TEST_F(ThumbnailerRunnerTest, testFindThumbnailerMacBundleLayout) {
	char templatePath[] = "/tmp/vengi-thumbappex.XXXXXX";
	ASSERT_NE(mkdtemp(templatePath), nullptr);
	const core::String root(templatePath);
	const core::String macOS = core::string::path(root, "Contents", "MacOS");
	const core::String appexMacOS =
		core::string::path(root, "Contents", "PlugIns", "vengi-voxelthumb.appex", "Contents", "MacOS");
	ASSERT_TRUE(io::Filesystem::sysCreateDir(macOS, true));
	ASSERT_TRUE(io::Filesystem::sysCreateDir(appexMacOS, true));

	const core::String modulePath = core::string::path(appexMacOS, "vengi-voxelthumb");
	const core::String thumbnailerPath = core::string::path(macOS, "vengi-thumbnailer");

	FILE *moduleFile = fopen(modulePath.c_str(), "w");
	ASSERT_NE(moduleFile, nullptr);
	fputs("module", moduleFile);
	fclose(moduleFile);

	FILE *thumbFile = fopen(thumbnailerPath.c_str(), "w");
	ASSERT_NE(thumbFile, nullptr);
	fputs("#!/bin/sh\nexit 0\n", thumbFile);
	fclose(thumbFile);
	ASSERT_EQ(chmod(thumbnailerPath.c_str(), 0755), 0);

	const core::String found = thumbnailer::findThumbnailerExecutable(modulePath);
	EXPECT_EQ(core::string::sanitizePath(thumbnailerPath), core::string::sanitizePath(found));

	io::Filesystem::sysRemoveDir(root, true);
}

TEST_F(ThumbnailerRunnerTest, testRenderThumbnailSuccess) {
	char templatePath[] = "/tmp/vengi-thumbrender.XXXXXX";
	ASSERT_NE(mkdtemp(templatePath), nullptr);
	const core::String dir(templatePath);
	const core::String fakeThumbnailer = core::string::path(dir, "vengi-thumbnailer");
	const core::String inputFile = core::string::path(dir, "input.vengi");
	const core::String outputFile = core::string::path(dir, "out.png");

	FILE *input = fopen(inputFile.c_str(), "w");
	ASSERT_NE(input, nullptr);
	fputs("dummy", input);
	fclose(input);

	FILE *script = fopen(fakeThumbnailer.c_str(), "w");
	ASSERT_NE(script, nullptr);
	// Mimic the CLI: write an empty PNG-like file to --output
	fputs("#!/bin/sh\n"
		  "out=\"\"\n"
		  "while [ $# -gt 0 ]; do\n"
		  "  if [ \"$1\" = \"--output\" ]; then shift; out=\"$1\"; fi\n"
		  "  shift\n"
		  "done\n"
		  "printf 'png' > \"$out\"\n"
		  "exit 0\n",
		  script);
	fclose(script);
	ASSERT_EQ(chmod(fakeThumbnailer.c_str(), 0755), 0);

	thumbnailer::RenderRequest request;
	request.thumbnailerPath = fakeThumbnailer;
	request.inputFile = inputFile;
	request.outputFile = outputFile;
	request.workingDirectory = dir;
	request.size = 64;
	request.timeoutMs = 5000;

	core::String error;
	ASSERT_TRUE(thumbnailer::renderThumbnail(request, &error)) << error.c_str();
	EXPECT_TRUE(io::Filesystem::sysExists(outputFile));

	io::Filesystem::sysRemoveDir(dir, true);
}

TEST_F(ThumbnailerRunnerTest, testRenderThumbnailTimeout) {
	char templatePath[] = "/tmp/vengi-thumbtimeout.XXXXXX";
	ASSERT_NE(mkdtemp(templatePath), nullptr);
	const core::String dir(templatePath);
	const core::String fakeThumbnailer = core::string::path(dir, "vengi-thumbnailer");
	const core::String inputFile = core::string::path(dir, "input.vengi");
	const core::String outputFile = core::string::path(dir, "out.png");

	FILE *input = fopen(inputFile.c_str(), "w");
	ASSERT_NE(input, nullptr);
	fputs("dummy", input);
	fclose(input);

	FILE *script = fopen(fakeThumbnailer.c_str(), "w");
	ASSERT_NE(script, nullptr);
	fputs("#!/bin/sh\nsleep 5\nexit 0\n", script);
	fclose(script);
	ASSERT_EQ(chmod(fakeThumbnailer.c_str(), 0755), 0);

	thumbnailer::RenderRequest request;
	request.thumbnailerPath = fakeThumbnailer;
	request.inputFile = inputFile;
	request.outputFile = outputFile;
	request.workingDirectory = dir;
	request.size = 64;
	request.timeoutMs = 200;

	core::String error;
	EXPECT_FALSE(thumbnailer::renderThumbnail(request, &error));
	EXPECT_FALSE(error.empty());

	io::Filesystem::sysRemoveDir(dir, true);
}
