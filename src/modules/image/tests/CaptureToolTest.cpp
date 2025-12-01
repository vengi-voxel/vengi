/**
 * @file
 */

#include "image/CaptureTool.h"
#include "app/tests/AbstractTest.h"
#include "image/Image.h"
#include "io/Filesystem.h"
#include "core/StandardLib.h"

namespace image {

class CaptureToolTest : public app::AbstractTest {
};

TEST_F(CaptureToolTest, testRecordAVI) {
	CaptureTool tool(CaptureType::AVI);
	const core::String filename = "test.avi";
	ASSERT_TRUE(tool.startRecording(filename.c_str(), 64, 64));
	EXPECT_TRUE(tool.isRecording());

	image::ImagePtr img = image::createEmptyImage("frame");
	uint8_t buffer[64 * 64 * 4];
	core_memset(buffer, 255, sizeof(buffer));
	img->loadRGBA(buffer, 64, 64);

	tool.enqueueFrame(img);
	tool.enqueueFrame(img);

	EXPECT_TRUE(tool.stopRecording());
	EXPECT_FALSE(tool.isRecording());
	EXPECT_TRUE(tool.flush());
	EXPECT_TRUE(tool.hasFinished());

	EXPECT_TRUE(_testApp->filesystem()->exists(filename));
}

TEST_F(CaptureToolTest, testRecordMPEG2) {
	CaptureTool tool(CaptureType::MPEG2);
	const core::String filename = "test.mpg";
	ASSERT_TRUE(tool.startRecording(filename.c_str(), 64, 64));
	EXPECT_TRUE(tool.isRecording());

	image::ImagePtr img = image::createEmptyImage("frame");
	uint8_t buffer[64 * 64 * 4];
	core_memset(buffer, 255, sizeof(buffer));
	img->loadRGBA(buffer, 64, 64);

	tool.enqueueFrame(img);
	tool.enqueueFrame(img);

	EXPECT_TRUE(tool.stopRecording());
	EXPECT_FALSE(tool.isRecording());
	EXPECT_TRUE(tool.flush());
	EXPECT_TRUE(tool.hasFinished());

	EXPECT_TRUE(_testApp->filesystem()->exists(filename));
}

TEST_F(CaptureToolTest, testAbort) {
	CaptureTool tool(CaptureType::AVI);
	const core::String filename = "test_abort.avi";
	ASSERT_TRUE(tool.startRecording(filename.c_str(), 64, 64));
	EXPECT_TRUE(tool.isRecording());

	image::ImagePtr img = image::createEmptyImage("frame");
	uint8_t buffer[64 * 64 * 4];
	core_memset(buffer, 255, sizeof(buffer));
	img->loadRGBA(buffer, 64, 64);

	tool.enqueueFrame(img);
	tool.abort();
	EXPECT_FALSE(tool.isRecording());
}

} // namespace image
