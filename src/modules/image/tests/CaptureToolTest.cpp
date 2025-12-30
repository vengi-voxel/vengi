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
	CaptureTool tool;
	const char *filename = "test.avi";
	ASSERT_TRUE(tool.startRecording(filename, 64, 64));
	EXPECT_EQ(tool.type(), CaptureType::AVI);
	EXPECT_TRUE(tool.isRecording());

	const image::ImagePtr &img = createImage();
	tool.enqueueFrame(img, 0.0);
	tool.enqueueFrame(img, 1.0);

	EXPECT_TRUE(tool.stopRecording());
	EXPECT_FALSE(tool.isRecording());
	EXPECT_TRUE(tool.flush());
	EXPECT_TRUE(tool.hasFinished());

	EXPECT_TRUE(_testApp->filesystem()->exists(filename));
}

TEST_F(CaptureToolTest, testRecordMPEG2) {
	CaptureTool tool;
	const char *filename = "test.mpg";
	ASSERT_TRUE(tool.startRecording(filename, 64, 64));
	EXPECT_EQ(tool.type(), CaptureType::MPEG2);
	EXPECT_TRUE(tool.isRecording());

	const image::ImagePtr &img = createImage();
	tool.enqueueFrame(img, 0.0);
	tool.enqueueFrame(img, 1.0);

	EXPECT_TRUE(tool.stopRecording());
	EXPECT_FALSE(tool.isRecording());
	EXPECT_TRUE(tool.flush());
	EXPECT_TRUE(tool.hasFinished());

	EXPECT_TRUE(_testApp->filesystem()->exists(filename));
}

TEST_F(CaptureToolTest, testAbort) {
	CaptureTool tool;
	const char *filename = "test_abort.avi";
	ASSERT_TRUE(tool.startRecording(filename, 64, 64));
	EXPECT_EQ(tool.type(), CaptureType::AVI);
	EXPECT_TRUE(tool.isRecording());

	const image::ImagePtr &img = createImage();
	tool.enqueueFrame(img, 0.0);
	tool.abort();
	EXPECT_FALSE(tool.isRecording());
}

TEST_F(CaptureToolTest, testRecordGIF) {
	CaptureTool tool;
	const char *filename = "test.gif";
	ASSERT_TRUE(tool.startRecording(filename, 64, 64));
	EXPECT_EQ(tool.type(), CaptureType::GIF);
	EXPECT_TRUE(tool.isRecording());

	const image::ImagePtr &img = createImage();
	tool.enqueueFrame(img, 0.0);
	tool.enqueueFrame(img, 1.0);

	while (tool.pendingFrames() > 0) {
		app::App::getInstance()->wait(10);
	}
	EXPECT_TRUE(tool.stopRecording());
	EXPECT_FALSE(tool.isRecording());
	EXPECT_TRUE(tool.flush());
	EXPECT_TRUE(tool.hasFinished());

	EXPECT_TRUE(_testApp->filesystem()->exists(filename));
}

TEST_F(CaptureToolTest, testDefaultCaptureFps) {
	EXPECT_EQ(defaultCaptureFps(CaptureType::GIF), 10);
	EXPECT_EQ(defaultCaptureFps(CaptureType::AVI), 25);
	EXPECT_EQ(defaultCaptureFps(CaptureType::MPEG2), 25);
}

TEST_F(CaptureToolTest, testShouldCaptureFrame) {
	CaptureTool tool;
	ASSERT_TRUE(tool.startRecording("test_skip.avi", 64, 64));
	EXPECT_EQ(tool.fps(), 25);

	// first frame should always be captured
	EXPECT_TRUE(tool.shouldCaptureFrame(0.0));

	const image::ImagePtr &img = createImage();
	tool.enqueueFrame(img, 0.0);

	// frame arriving too soon should be skipped (1/25 = 0.04s interval)
	EXPECT_FALSE(tool.shouldCaptureFrame(0.01));
	EXPECT_FALSE(tool.shouldCaptureFrame(0.03));

	// frame arriving at or after the interval should be captured
	EXPECT_TRUE(tool.shouldCaptureFrame(0.04));
	EXPECT_TRUE(tool.shouldCaptureFrame(0.1));

	tool.abort();
}

TEST_F(CaptureToolTest, testShouldCaptureFrameGIF) {
	CaptureTool tool;
	ASSERT_TRUE(tool.startRecording("test_skip.gif", 64, 64));
	EXPECT_EQ(tool.fps(), 10);

	// first frame should always be captured
	EXPECT_TRUE(tool.shouldCaptureFrame(0.0));

	const image::ImagePtr &img = createImage();
	tool.enqueueFrame(img, 0.0);

	// 1/10 = 0.1s interval - frames before that should be skipped
	EXPECT_FALSE(tool.shouldCaptureFrame(0.05));
	EXPECT_FALSE(tool.shouldCaptureFrame(0.09));
	EXPECT_TRUE(tool.shouldCaptureFrame(0.1));
	EXPECT_TRUE(tool.shouldCaptureFrame(0.2));

	tool.abort();
}

} // namespace image
