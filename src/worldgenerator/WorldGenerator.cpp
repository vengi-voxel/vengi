#include "WorldGenerator.h"
#include "sauce/WorldGeneratorInjector.h"
#include "core/Var.h"

WorldGenerator::WorldGenerator(voxel::WorldPtr world, core::EventBusPtr eventBus, core::TimeProviderPtr timeProvider, io::FilesystemPtr filesystem) :
		App(filesystem, eventBus, 15681), _world(world), _timeProvider(timeProvider), _seed(0L), _size(0) {
	init("engine", "worldgenerator");
}

core::AppState WorldGenerator::onInit() {
	core::AppState state = App::onInit();
	const core::VarPtr& seed = core::Var::get("seed");
	const core::VarPtr& size = core::Var::get("size");

	if (size->strVal().empty()) {
		Log::error("No size specified: -set size <size>");
		return core::AppState::Cleanup;
	} else if (seed->strVal().empty()) {
		Log::error("No seed specified: -set seed <seed>");
		return core::AppState::Cleanup;
	}
	_seed = seed->longVal();
	_size = size->intVal();
	return state;
}

core::AppState WorldGenerator::onRunning() {
	core::AppState state = core::App::onRunning();

	class ProgressMonitor: public util::IProgressMonitor {
	public:
		void step(int steps = 1) override {
			IProgressMonitor::step(steps);
			Log::info("max: %i, steps: %i => %f\r", _max, _steps, progress());
		}
		void done() override {
			Log::info("\ndone");
		}
	} monitor;

	unsigned long start = _timeProvider->currentTime();
	_world->create(_seed, _size, &monitor);
	if (!_world->save(_seed)) {
		Log::error("Failed to save the world for seed %li", _seed);
	} else {
		Log::info("World for seed %li created", _seed);
	}
	unsigned long end = _timeProvider->currentTime();
	unsigned long delta = end - start;
	Log::info("World generating process took %lu milliseconds", delta);
	return state;
}

int main(int argc, char *argv[]) {
	getInjector()->get<WorldGenerator>()->startMainLoop(argc, argv);
	return EXIT_SUCCESS;
}
