/**
 * @file
 */

#pragma once

#include "video/WindowedApp.h"
#include "io/File.h"

/**
 * @brief This tool is able to generate thumbnails for all supported voxel formats
 *
 * @ingroup Tools
 */
class Thumbnailer: public video::WindowedApp {
private:
	using Super = video::WindowedApp;

	io::FilePtr _infile;
	core::String _outfile;
	int _outputSize = 128;

	bool renderVolume();
	bool saveEmbeddedScreenshot();
public:
	Thumbnailer(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onRunning() override;
	app::AppState onCleanup() override;
};
