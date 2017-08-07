/**
 * @file
 */

#pragma once

#include "core/App.h"
#include "compute/Types.h"
#include <simplecpp.h>
#include <vector>

/**
 * @brief This tool validates the shaders and generated c++ code for them.
 */
class ComputeShaderTool: public core::App {
protected:
	std::string _namespaceSrc;
	std::string _sourceDirectory;
	std::string _shaderDirectory;
	std::string _computeFilename;
	std::string _shaderTemplateFile;
	std::string _name;

	struct Parameter {
		std::string qualifier;
		std::string type;
		std::string name;
		compute::BufferFlag flags = compute::BufferFlag::ReadWrite;
	};

	struct ReturnValue {
		std::string type;
	};

	struct Kernel {
		std::string name;
		std::vector<Parameter> parameters;
		ReturnValue returnValue;
	};

	struct Struct {
		std::string name;
		std::vector<Parameter> parameters;
	};

	std::vector<Kernel> _kernels;
	std::vector<Struct> _structs;

	const simplecpp::Token *parseKernel(const simplecpp::Token *tok);
	const simplecpp::Token *parseStruct(const simplecpp::Token *tok);
	bool parse(const std::string& src);
	void generateSrc();
	static bool validate(Kernel& kernel);
public:
	ComputeShaderTool(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);
	~ComputeShaderTool();

	core::AppState onRunning() override;
};
