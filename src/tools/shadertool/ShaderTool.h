/**
 * @file
 */

#pragma once

#include "core/App.h"

class ShaderTool: public core::App {
protected:
	struct Variable {
		enum Type {
			FLOAT, BOOL, INT, VEC2, VEC3, VEC4, MAT, SAMPLER2D, SAMPLER2DSHADOW
		};
		Type type;
		std::string name;
	};

	struct ShaderStruct {
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
	bool parse(const std::string& src, bool vertex);
	Variable::Type getType(const std::string& type) const;
	void generateSrc() const;
public:
	ShaderTool(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);
	~ShaderTool();

	core::AppState onRunning() override;
};
