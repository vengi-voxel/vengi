/**
 * @file
 */

#pragma once

#include "core/CommandlineApp.h"
#include "Types.h"

/**
 * @brief This tool validates the GLSL shaders and generates c++ code for them.
 *
 * @ingroup Tools
 */
class ShaderTool: public core::CommandlineApp {
private:
	using Super = core::CommandlineApp;
protected:
	ShaderStruct _shaderStruct;
	core::String _namespaceSrc;
	core::String _sourceDirectory;
	core::String _postfix;
	core::String _shaderDirectory;
	core::String _headerTemplateFile;
	core::String _sourceTemplateFile;
	core::String _glslangValidatorBin;
	core::String _uniformBufferTemplateFile;
	core::String _constantsTemplateFile;
	core::String _shaderfile;
	core::String _shaderpath;
	std::vector<core::String> _includes;
	std::vector<core::String> _includeDirs;

	bool parse(const core::String& src, bool vertex);
	void validate(const core::String& name);
	std::pair<core::String, bool> getSource(const core::String& file) const;
	bool printInfo();
public:
	ShaderTool(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	core::AppState onConstruct() override;
	core::AppState onRunning() override;
};
