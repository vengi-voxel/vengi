/**
 * @file
 */

#pragma once

#include "image/Image.h"
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

protected:
	virtual bool saveImage(const image::ImagePtr &image);

public:
	Thumbnailer(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider);

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onRunning() override;
	app::AppState onCleanup() override;
};
