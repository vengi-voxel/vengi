/**
 * @file
 */

#pragma once

#include "core/App.h"
#include "core/Tokenizer.h"

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
			SAMPLERCUBEMAP, SAMPLER2DARRAYSHADOW, SAMPLER2DARRAY,
			SAMPLER1DSHADOW, SAMPLER2DSHADOW, MAX
		};
		Type type;
		std::string name;
		int arraySize = 0;

		inline bool isSingleInteger() const {
			return isSampler() || type == Variable::INT || type == Variable::UNSIGNED_INT;
		}

		inline bool isSampler() const {
			return type == Variable::SAMPLER1D || type == Variable::SAMPLER2D || type == Variable::SAMPLER3D
			 || type == Variable::SAMPLER2DSHADOW || type == Variable::SAMPLER1DSHADOW || type == Variable::SAMPLERCUBEMAP;
		}

		inline bool isInteger() const {
			return type == Variable::UNSIGNED_INT || type == Variable::INT || type == Variable::IVEC2 || type == Variable::IVEC3 || type == Variable::IVEC4;
		}
	};

	enum PassBy {
		Value,
		Reference,
		Pointer
	};

	struct Types {
		ShaderTool::Variable::Type type;
		int components;
		const char* ctype;
		PassBy passBy;
		const char* glsltype;
	};

	static const Types cTypes[];

	struct UniformBlock {
		std::string name;
		std::vector<Variable> members;
	};

	enum class BlockLayout {
		unknown,
		std140,
		std430
	};

	struct Layout {
		int binding = 0;
		int components = 0;
		int offset = 0;
		int index = 0;
		BlockLayout blockLayout = BlockLayout::unknown;
	};

	struct ShaderStruct {
		std::string name;
		std::string filename;
		// both
		std::vector<Variable> uniforms;
		std::vector<UniformBlock> uniformBlocks;
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
	std::string _uniformBufferTemplateFile;

	bool parseLayout(Layout& layout, core::Tokenizer& tok);
	bool parse(const std::string& src, bool vertex);
	Variable::Type getType(const std::string& type) const;
	std::string std140Align(const Variable& v) const;
	size_t std140Size(const Variable& v) const;
	std::string std140Padding(const Variable& v, int& padding) const;
	int getComponents(const Variable::Type type) const;
	std::string uniformSetterPostfix(const Variable::Type type, int amount) const;
	void generateSrc();
public:
	ShaderTool(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);
	~ShaderTool();

	core::AppState onConstruct() override;
	core::AppState onRunning() override;
};
