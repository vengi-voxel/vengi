/**
 * @file
 */
#include "NoiseTool2.h"
#include "io/Filesystem.h"
#include "core/Color.h"
#include "NodeGraph.h"
#include "voxel/MaterialColor.h"

NoiseTool2::NoiseTool2(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
}

void NoiseTool2::onRenderUI() {
	ImGui::SetNextWindowPosCenter(ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(dimension().x, dimension().y), ImGuiSetCond_FirstUseEver);
	ImGui::Begin("Node graph", &_windowOpened);
	showNodeGraph();
	ImGui::End();
	if (!_windowOpened) {
		requestQuit();
	}
}

core::AppState NoiseTool2::onConstruct() {
	const core::AppState state = Super::onConstruct();
	return state;
}

core::AppState NoiseTool2::onCleanup() {
	core::AppState state = Super::onCleanup();
	shutdownNodeGraph();
	return state;
}

core::AppState NoiseTool2::onInit() {
	core::AppState state = Super::onInit();
	_logLevelVar->setVal(std::to_string(SDL_LOG_PRIORITY_DEBUG));
	Log::init();
	if (state != core::AppState::Running) {
		return state;
	}

	video::clearColor(::core::Color::Black);
	//video::enableDebug(video::DebugSeverity::Medium);

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return core::AppState::InitFailure;
	}

	_camera.setFarPlane(4000.0f);

	return state;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	NoiseTool2 app(metric, filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
