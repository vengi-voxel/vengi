/**
 * @file
 */

#include "AbstractGLTest.h"
#include "image/Image.h"
#include "video/WindowedApp.h"

namespace video {

class WindowedAppTest : public AbstractGLTest {};

TEST_F(WindowedAppTest, testSetWindowIconFromImage) {
	if (IsSkipped()) {
		return;
	}
	const uint8_t rgba[] = {255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 0, 255};
	image::ImagePtr icon = image::createEmptyImage("test-icon");
	ASSERT_TRUE(icon->loadRGBA(rgba, 2, 2));
	ASSERT_TRUE(WindowedApp::setWindowIcon(_window, icon));
}

TEST_F(WindowedAppTest, testSetWindowIconRejectsInvalid) {
	if (IsSkipped()) {
		return;
	}
	ASSERT_FALSE(WindowedApp::setWindowIcon(_window, image::ImagePtr()));
	ASSERT_FALSE(WindowedApp::setWindowIcon(nullptr, image::createEmptyImage("empty")));
}

} // namespace video
