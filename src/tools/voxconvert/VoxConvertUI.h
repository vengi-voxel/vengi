/**
 * @file
 */

#pragma once

#include "ui/IMGUIApp.h"

/**
 * @brief This is a basic ui for voxconvert
 *
 * @ingroup Tools
 */
class VoxConvertUI : public ui::IMGUIApp {
private:
	using Super = ui::IMGUIApp;
	core::String _output;
	core::String _source;
	core::Path _voxconvertBinary {"vengi-voxconvert"};
	core::String _targetFile;
	bool _targetFileExists = false;
	bool _overwriteTargetFile = false;
	bool _dirtyTargetFile = false;
	float _filterFormatTextWidth = -1.0f;
	core::DynamicArray<io::FormatDescription> _filterEntries;
	core::VarPtr _lastTarget;
	core::VarPtr _lastSource;

	void genericOptions(const io::FormatDescription *desc);
	void sourceOptions(const io::FormatDescription *desc) const;
	void targetOptions(const io::FormatDescription *desc) const;

protected:
	app::AppState onConstruct() override;
	app::AppState onInit() override;
	void onRenderUI() override;

public:
	VoxConvertUI(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider);
};
