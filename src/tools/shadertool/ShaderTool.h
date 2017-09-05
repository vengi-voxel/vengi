/**
 * @file
 */

#pragma once

#include "core/ConsoleApp.h"
#include "core/Tokenizer.h"
#include <simplecpp.h>

class TokenIterator {
private:
	const simplecpp::TokenList* _tokenList = nullptr;
	const simplecpp::Token *_tok = nullptr;
public:
	void init(const simplecpp::TokenList* tokenList) {
		_tokenList = tokenList;
		_tok = _tokenList->cfront();
	}

	inline bool hasNext() const {
		return _tok != nullptr;
	}

	inline std::string next() {
		const std::string& token = _tok->str;
		_tok = _tok->next;
		return token;
	}

	inline std::string prev() {
		_tok = _tok->previous;
		return _tok->str;
	}

	inline int line() const {
		if (!_tok) {
			return -1;
		}
		return _tok->location.line;
	}

	inline std::string peekNext() const {
		if (!_tok) {
			return "";
		}
		return _tok->str;
	}
};

/**
 * @brief This tool validates the shaders and generated c++ code for them.
 */
class ShaderTool: public core::ConsoleApp {
private:
	using Super = core::ConsoleApp;
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
			// TODO: atomics
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

	enum class PrimitiveType {
		None,
		Points,
		Lines,
		LinesAdjacency,
		Triangles,
		TrianglesAdjacency,
		LineStrip,
		TriangleStrip,
		Max
	};
	static const char* PrimitiveTypeStr[];

	struct Layout {
		int binding = 0;
		int components = 0;
		int offset = 0;
		int index = 0;
		int location = -1;
		int transformFeedbackOffset = -1; // 4-4
		int transformFeedbackBuffer = -1; // 4-4
		int tesselationVertices = -1; // 4.0
		int maxGeometryVertices = -1; // 4.0
		bool originUpperLeft = false; // 4.0
		bool pixelCenterInteger = false; // 4.0
		bool earlyFragmentTests = false; // 4.2
		PrimitiveType primitiveType = PrimitiveType::None;
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
	TokenIterator _tok;
	Layout _layout;
	std::string _namespaceSrc;
	std::string _sourceDirectory;
	std::string _shaderDirectory;
	std::string _shaderTemplateFile;
	std::string _uniformBufferTemplateFile;
	std::string _shaderfile;
	std::string _currentSource;

	PrimitiveType layoutPrimitiveType(const std::string& token) const;
	bool parseLayout();
	bool parse(const std::string& src, bool vertex);
	Variable::Type getType(const std::string& type) const;

	std::string std140Align(const Variable& v) const;
	size_t std140Size(const Variable& v) const;
	std::string std140Padding(const Variable& v, int& padding) const;

	std::string std430Align(const Variable& v) const;
	size_t std430Size(const Variable& v) const;
	std::string std430Padding(const Variable& v, int& padding) const;

	std::string typeAlign(const Variable& v) const;
	size_t typeSize(const Variable& v) const;
	std::string typePadding(const Variable& v, int& padding) const;

	int getComponents(const Variable::Type type) const;
	std::string uniformSetterPostfix(const Variable::Type type, int amount) const;
	void generateSrc();
public:
	ShaderTool(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	core::AppState onConstruct() override;
	core::AppState onRunning() override;
};
