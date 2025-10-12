/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include "video/WindowedApp.h"

/**
 * @brief This tool is able to generate thumbnails for all supported voxel formats
 *
 * @ingroup Tools
 *
 * On linux the screenshots are usually saved in @c $XDG_CACHE_HOME/thumbnails or if not available in
 * @c $HOME/.cache/thumbnails
 *
 * See @link https://specifications.freedesktop.org/thumbnail-spec/thumbnail-spec-latest.html for more details
 */
class Thumbnailer: public video::WindowedApp {
private:
	using Super = video::WindowedApp;

	core::String _outfile;

protected:
	virtual bool saveImage(const image::ImagePtr &image);
	void printUsageHeader() const override;

public:
	Thumbnailer(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider);

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onRunning() override;
	app::AppState onCleanup() override;
};
