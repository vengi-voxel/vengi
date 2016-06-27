/**
 * @file
 */

#pragma once

#include "core/App.h"

class ShaderTool: public core::App {
protected:
	struct Variable {
		enum Type {
			FLOAT, UNSIGNED_INT, INT, VEC2, VEC3, VEC4, MAT, SAMPLER2D, SAMPLER2DSHADOW, MAX
		};
		Type type;
		std::string name;
		int arraySize = 0;
	};

	enum PassBy {
		Value,
		Reference,
		Pointer
	};

	struct Types {
		ShaderTool::Variable::Type type;
		const char* ctype;
		PassBy passBy;
	};

	static const Types cTypes[];

	struct ShaderStruct {
		std::string name;
		std::string filename;
		// both
		std::vector<Variable> uniforms;
		// vertex only
		std::vector<Variable> attributes;
		// vertex only
		std::vector<Variable> varyings;
		// fragment only
		std::vector<Variable> outs;
	};
	ShaderStruct _shaderStruct;
	std::string _namespaceSrc;
	std::string _sourceDirectory;
	std::string _shaderDirectory;
	std::string _shaderTemplateFile;

	bool parse(const std::string& src, bool vertex);
	Variable::Type getType(const std::string& type) const;
	std::string uniformSetterPostfix(const Variable::Type type, int amount) const;
	void generateSrc() const;
public:
	ShaderTool(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);
	~ShaderTool();

	core::AppState onRunning() override;
};
