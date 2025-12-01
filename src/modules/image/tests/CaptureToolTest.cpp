/**
 * @file
 */

#include "image/CaptureTool.h"
#include "app/tests/AbstractTest.h"
#include "core/StandardLib.h"
#include "image/Image.h"
#include "io/Filesystem.h"

namespace image {

class CaptureToolTest : public app::AbstractTest {
protected:
	image::ImagePtr createImage() {
		image::ImagePtr img = image::createEmptyImage("frame");
		uint8_t buffer[64 * 64 * 4];
		core_memset(buffer, 255, sizeof(buffer));
		img->loadRGBA(buffer, 64, 64);
		return img;
	}
};

TEST_F(CaptureToolTest, testRecordAVI) {
	CaptureTool tool(CaptureType::AVI);
	const char *filename = "test.avi";
	ASSERT_TRUE(tool.startRecording(filename, 64, 64));
	EXPECT_TRUE(tool.isRecording());

	const image::ImagePtr &img = createImage();
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
	const char *filename = "test.mpg";
	ASSERT_TRUE(tool.startRecording(filename, 64, 64));
	EXPECT_TRUE(tool.isRecording());

	const image::ImagePtr &img = createImage();
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
	const char *filename = "test_abort.avi";
	ASSERT_TRUE(tool.startRecording(filename, 64, 64));
	EXPECT_TRUE(tool.isRecording());

	const image::ImagePtr &img = createImage();
	tool.enqueueFrame(img);
	tool.abort();
	EXPECT_FALSE(tool.isRecording());
}

} // namespace image
