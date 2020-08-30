/**
 * @file
 */

#pragma once

#include <benchmark/benchmark.h>
#include "app/CommandlineApp.h"
#include "io/Filesystem.h"
#include "core/EventBus.h"
#include "core/TimeProvider.h"

namespace core {

class AbstractBenchmark : public benchmark::Fixture {
private:
	class BenchmarkApp: public core::CommandlineApp {
		friend class AbstractBenchmark;
	protected:
		using Super = core::CommandlineApp;
		AbstractBenchmark* _benchmark = nullptr;
	public:
		BenchmarkApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, AbstractBenchmark* benchmark);
		virtual ~BenchmarkApp();

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
