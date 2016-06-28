#include "TestDepthBuffer.h"
#include "video/GLDebug.h"

TestDepthBuffer::TestDepthBuffer(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		video::WindowedApp(filesystem, eventBus, 21000) {
}

TestDepthBuffer::~TestDepthBuffer() {
}

core::AppState TestDepthBuffer::onInit() {
	const core::AppState state = video::WindowedApp::onInit();

	GLDebug::enable(GLDebug::Medium);

	_depthBuffer.init(_width, _height);
	return state;
}

core::AppState TestDepthBuffer::onRunning() {
	return video::WindowedApp::onRunning();
}

core::AppState TestDepthBuffer::onCleanup() {
	_depthBuffer.shutdown();
	return video::WindowedApp::onCleanup();
}

int main(int argc, char *argv[]) {
	return core::getApp<TestDepthBuffer>()->startMainLoop(argc, argv);
}
