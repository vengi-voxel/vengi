/**
 * @file
 */

#pragma once

#include "core/String.h"

/**
 * @brief Shared helper that locates and runs @c vengi-thumbnailer for platform shell integrations
 * (Windows Explorer COM, macOS Quick Look). Keeps GPU/window setup out of the host process.
 */
namespace thumbnailer {

struct RenderRequest {
	/** Absolute path of the platform plugin/extension binary (used for relative lookup). */
	core::String modulePath;
	/** Optional explicit path to @c vengi-thumbnailer; empty = search relative to @c modulePath. */
	core::String thumbnailerPath;
	core::String inputFile;
	core::String outputFile;
	/** Working directory for the renderer (also used as isolated @c APP_HOMEPATH). */
	core::String workingDirectory;
	int size = 128;
	int timeoutMs = 30000;
	bool useSceneCamera = true;
};

/**
 * @brief Resolve @c vengi-thumbnailer next to the plugin or inside a macOS app bundle layout
 * @param[in] modulePath Absolute path to the plugin/extension executable or library
 * @return Absolute path or empty string if not found
 */
core::String findThumbnailerExecutable(const core::String &modulePath);

/**
 * @brief Run the thumbnailer CLI and write a PNG to @c request.outputFile
 * @return @c true on exit code 0 and an existing output file
 */
bool renderThumbnail(const RenderRequest &request, core::String *errorMessage = nullptr);

} // namespace thumbnailer
