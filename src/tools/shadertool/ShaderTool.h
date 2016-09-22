/**
 * @file
 */

#pragma once

#include "core/App.h"

/**
 * @brief This tool validates the shaders and generated c++ code for them.
 */
class ShaderTool: public core::App {
protected:
	struct Variable {
		enum Type {
			DOUBLE, FLOAT, UNSIGNED_INT, INT, BOOL,
			DVEC2, DVEC3, DVEC4, BVEC2, BVEC3, BVEC4,
			UVEC2, UVEC3, UVEC4, IVEC2, IVEC3, IVEC4,
			VEC2, VEC3, VEC4,
			MAT2, MAT3, MAT4, MAT3X4, MAT4X3,
			SAMPLER1D, SAMPLER2D, SAMPLER3D,
			SAMPLERCUBEMAP,
			SAMPLER1DSHADOW, SAMPLER2DSHADOW, MAX
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
		int typeSize;
		const char* ctype;
		PassBy passBy;
		const char* glsltype;
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
	int getComponents(const Variable::Type type) const;
	std::string uniformSetterPostfix(const Variable::Type type, int amount) const;
	void generateSrc() const;
public:
	ShaderTool(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);
	~ShaderTool();

	core::AppState onRunning() override;
};
