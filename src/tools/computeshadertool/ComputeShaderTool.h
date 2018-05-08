/**
 * @file
 */

#pragma once

#include "core/ConsoleApp.h"
#include "Types.h"
#include <simplecpp.h>
#include <vector>

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
class ComputeShaderTool: public core::ConsoleApp {
private:
	using Super = core::ConsoleApp;
protected:
	std::string _namespaceSrc;
	std::string _sourceDirectory;
	std::string _postfix;
	std::string _shaderDirectory;
	std::string _computeFilename;
	std::string _shaderTemplateFile;
	std::string _name;
	std::vector<computeshadertool::Kernel> _kernels;
	std::vector<computeshadertool::Struct> _structs;
	std::map<std::string, std::string> _constants;
	std::vector<std::string> _includeDirs;

	std::string getSource(const std::string& file) const;
	bool parse(const std::string& src);
public:
	ComputeShaderTool(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);
	~ComputeShaderTool();

	core::AppState onConstruct() override;
	core::AppState onRunning() override;
};
