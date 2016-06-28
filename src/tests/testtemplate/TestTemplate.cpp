#include "TestTemplate.h"
#include "core/AppModule.h"
#include "video/GLFunc.h"
#include "core/Color.h"

TestTemplate::TestTemplate(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		Super(filesystem, eventBus, 21000) {
}

TestTemplate::~TestTemplate() {
}

core::AppState TestTemplate::onInit() {
	const core::AppState state = Super::onInit();
	GLDebug::enable(GLDebug::Medium);

	const glm::vec4& color = ::core::Color::Red;
	glClearColor(color.r, color.g, color.b, color.a);

	return state;
}

core::AppState TestTemplate::onRunning() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	return core::AppState::Cleanup;
}

int main(int argc, char *argv[]) {
	return core::getApp<TestTemplate>()->startMainLoop(argc, argv);
}
