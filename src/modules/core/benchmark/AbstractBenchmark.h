#include <benchmark/benchmark.h>
#include "core/App.h"
#include "io/Filesystem.h"
#include "core/EventBus.h"
#include "core/TimeProvider.h"

namespace core {

class AbstractBenchmark : public benchmark::Fixture {
private:
	class BenchmarkApp: public core::App {
		friend class AbstractBenchmark;
	protected:
		AbstractBenchmark* _benchmark = nullptr;
	public:
		BenchmarkApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, AbstractBenchmark* benchmark);
		~BenchmarkApp();

		virtual core::AppState onInit() override;
		virtual core::AppState onCleanup() override;
	};

protected:
	BenchmarkApp *_benchmarkApp = nullptr;

	virtual void onCleanupApp() {
	}

	virtual bool onInitApp() {
		return true;
	}

public:
	virtual void SetUp(benchmark::State& st) override;

	virtual void TearDown(benchmark::State& st) override;
};

}
