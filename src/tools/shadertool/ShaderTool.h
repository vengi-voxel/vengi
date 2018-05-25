/**
 * @file
 */

#pragma once

#include "core/ConsoleApp.h"
#include "Types.h"

/**
 * @brief This tool validates the GLSL shaders and generates c++ code for them.
 *
 * @ingroup Tools
 */
class ShaderTool: public core::ConsoleApp {
private:
	using Super = core::ConsoleApp;
protected:
	ShaderStruct _shaderStruct;
	std::string _namespaceSrc;
	std::string _sourceDirectory;
	std::string _postfix;
	std::string _shaderDirectory;
	std::string _headerTemplateFile;
	std::string _sourceTemplateFile;
	std::string _glslangValidatorBin;
	std::string _uniformBufferTemplateFile;
	std::string _shaderfile;
	std::string _shaderpath;
	std::vector<std::string> _includes;
	std::vector<std::string> _includeDirs;

	bool parse(const std::string& src, bool vertex);
	void validate(const std::string& name);
	std::string getSource(const std::string& file) const;
	bool printInfo();
public:
	ShaderTool(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	core::AppState onConstruct() override;
	core::AppState onRunning() override;
};
