/**
 * @file
 */

#pragma once

#include "video/WindowedApp.h"
#include "video/FrameBuffer.h"
#include "video/Texture.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "io/File.h"

/**
 * @brief This tool is able to generate thumbnails for all supported voxel formats
 *
 * @ingroup Tools
 */
class Thumbnailer: public video::WindowedApp {
private:
	using Super = video::WindowedApp;

	video::FrameBuffer _frameBuffer;

	io::FilePtr _infile;
	core::String _outfile;
	int _outputSize = 128;

	voxelrender::RawVolumeRenderer _renderer;

public:
	Thumbnailer(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onRunning() override;
	app::AppState onCleanup() override;
};
