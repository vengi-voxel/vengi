/**
 * @file
 */

#pragma once

#include "app/CommandlineApp.h"
#include "core/collection/StringMap.h"
#include "core/collection/List.h"
#include "Types.h"
#include <simplecpp.h>

/**
 * @brief This tool validates the compute shaders and generates c++ code for them.
 *
 * @li contains a C preprocessor (simplecpp/cppcheck).
 * @li detects the needed dimensions of the compute shader and generate worksizes with
 *  proper types to call the kernels.
 * @li converts OpenCL types into glm and stl types (basically just vector).
 * @li handles alignment and padding of types according to the OpenCL specification.
 * @li detects buffer flags like use-the-host-pointer(-luke) according to the alignment
 *  and size.
 * @li hides all the buffer creation/deletion mambo-jambo from the caller.
 * @li parses OpenCL structs and generate proper aligned C++ struct for them.
 *
 * @ingroup Tools
 * @ingroup Compute
 */
class ComputeShaderTool: public app::CommandlineApp {
private:
	using Super = app::CommandlineApp;
protected:
	core::String _namespaceSrc;
	core::String _sourceDirectory;
	core::String _postfix;
	core::String _shaderDirectory;
	core::String _computeFilename;
	core::String _shaderTemplateFile;
	core::String _name;
	core::List<computeshadertool::Kernel> _kernels;
	core::List<computeshadertool::Struct> _structs;
	core::StringMap<core::String> _constants;
	core::List<core::String> _includeDirs;

	std::pair<core::String, bool> getSource(const core::String& file) const;
	bool parse(const core::String& src);
public:
	ComputeShaderTool(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);
	~ComputeShaderTool();

	app::AppState onConstruct() override;
	app::AppState onRunning() override;
};
