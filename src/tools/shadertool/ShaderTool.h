/**
 * @file
 */

#pragma once

#include "app/CommandlineApp.h"
#include "Types.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicStringMap.h"
#include "core/collection/List.h"
#include "core/Pair.h"

/**
 * @brief This tool validates the GLSL shaders and generates c++ code for them.
 *
 * @ingroup Tools
 */
class ShaderTool: public app::CommandlineApp {
private:
	using Super = app::CommandlineApp;
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
	bool _spirv = false;
	core::String _spirvEnv;
	core::DynamicStringMap<int> _spirvVaryingLocations;
	int _spirvNextVaryingLocation = 0;
	core::List<core::String> _includes;
	core::List<core::String> _includeDirs;

	bool parse(const core::String& filename, const core::String& src, bool vertex);
	void validate(const core::String& name);
	bool compileSPIRV(const core::String& source, const core::String& shaderType, core::DynamicArray<uint32_t>& spirvBinary);
	core::Pair<core::String, bool> getSource(const core::String& file) const;
	bool printInfo();
public:
	ShaderTool(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider);

	app::AppState onConstruct() override;
	app::AppState onRunning() override;
};
