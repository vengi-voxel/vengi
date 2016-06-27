/**
 * @file
 */

#include "ShaderTool.h"
#include "sauce/ShaderToolInjector.h"
#include "core/App.h"
#include "core/Process.h"
#include "core/Tokenizer.h"
#include "video/Shader.h"

const ShaderTool::Types ShaderTool::cTypes[] = {
	{ ShaderTool::Variable::FLOAT,           "float",           Value },
	{ ShaderTool::Variable::INT,             "unsigned int",    Value },
	{ ShaderTool::Variable::INT,             "int",             Value },
	{ ShaderTool::Variable::VEC2,            "const glm::vec2", Reference },
	{ ShaderTool::Variable::VEC3,            "const glm::vec3", Reference },
	{ ShaderTool::Variable::VEC4,            "const glm::vec4", Reference },
	{ ShaderTool::Variable::MAT,             "const glm::mat4", Reference },
	{ ShaderTool::Variable::SAMPLER2D,       "int",             Value },
	{ ShaderTool::Variable::SAMPLER2DSHADOW, "int",             Value }
};

// TODO: move into src/modules/video/Shader.h.in and hand filename in via cmake
static const char* templateShader =
	"/**\n"
	" * @file\n"
	" */\n"
	"\n"
	"#pragma once\n"
	"\n"
	"#include \"video/Shader.h\"\n"
	"\n"
	"namespace $namespace$ {\n"
	"\n"
	"class $name$ : public video::Shader {\n"
	"public:\n"
	"	bool setup() {\n"
	"		if (!loadProgram(\"$filename$\")) {\n"
	"			return false;\n"
	"		}\n"
	"		$attributes$\n"
	"		$uniforms$\n"
	"		return true;\n"
	"	}\n"
	"$setters$"
	"};\n"
	"\n"
	"typedef std::shared_ptr<$name$> $name$Ptr;\n"
	"\n"
	"}\n";

ShaderTool::ShaderTool(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		core::App(filesystem, eventBus, 0) {
	init("engine", "shadertool");
	static_assert(Variable::MAX == SDL_arraysize(cTypes), "mismatch in glsl types");
}

ShaderTool::~ShaderTool() {
}

std::string ShaderTool::uniformSetterPostfix(const ShaderTool::Variable::Type type, int amount) const {
	switch (type) {
	case Variable::MAX:
		return "";
	case Variable::FLOAT:
		if (amount > 1) {
			return "fv";
		}
		return "f";
	case Variable::UNSIGNED_INT:
		if (amount > 1) {
			return "uiv";
		}
		return "ui";
	case Variable::INT:
		if (amount > 1) {
			return "iv";
		}
		return "i";
	case Variable::VEC2:
		if (amount > 1) {
			return "Vec2v";
		}
		return "Vec2";
	case Variable::VEC3:
		if (amount > 1) {
			return "Vec3v";
		}
		return "Vec3";
	case Variable::VEC4:
		if (amount > 1) {
			return "Vec4v";
		}
		return "Vec4";
	case Variable::MAT:
		return "Matrix";
	case Variable::SAMPLER2D:
		if (amount > 1) {
			return "iv";
		}
		return "i";
	case Variable::SAMPLER2DSHADOW:
		if (amount > 1) {
			return "iv";
		}
		return "i";
	}
	return "";
}

ShaderTool::Variable::Type ShaderTool::getType(const std::string& type) const {
	if (type == "float") {
		return Variable::FLOAT;
	} else if (type == "int") {
		return Variable::INT;
	} else if (type == "uint") {
		return Variable::INT;
	} else if (type == "vec2") {
		return Variable::VEC2;
	} else if (type == "vec3") {
		return Variable::VEC3;
	} else if (type == "vec4") {
		return Variable::VEC4;
	} else if (type == "uvec2") {
		return Variable::VEC2;
	} else if (type == "uvec3") {
		return Variable::VEC3;
	} else if (type == "uvec4") {
		return Variable::VEC4;
	} else if (type == "mat4") {
		return Variable::MAT;
	} else if (type == "sampler2D") {
		return Variable::SAMPLER2D;
	} else if (type == "sampler2DShadow") {
		return Variable::SAMPLER2DSHADOW;
	}
	core_assert_msg(false, "unknown type given: %s", type.c_str());
	return Variable::FLOAT;
}

void ShaderTool::generateSrc() const {
	for (const auto& v : _shaderStruct.uniforms) {
		Log::info("Found uniform of type %i with name %s", int(v.type), v.name.c_str());
	}
	for (const auto& v : _shaderStruct.attributes) {
		Log::info("Found attribute of type %i with name %s", int(v.type), v.name.c_str());
	}
	for (const auto& v : _shaderStruct.varyings) {
		Log::info("Found varying of type %i with name %s", int(v.type), v.name.c_str());
	}
	for (const auto& v : _shaderStruct.outs) {
		Log::info("Found out var of type %i with name %s", int(v.type), v.name.c_str());
	}

	std::string src(templateShader);
	std::string name = _shaderStruct.name + "Shader";

	std::vector<std::string> shaderNameParts;
	core::string::splitString(name, shaderNameParts, "_");
	std::string filename = "";
	for (std::string n : shaderNameParts) {
		if (n.length() > 1 || shaderNameParts.size() < 2) {
			n[0] = SDL_toupper(n[0]);
			filename += n;
		}
	}
	if (filename.empty()) {
		filename = name;
	}
	const std::string classname = filename;
	filename += ".h";
	src = core::string::replaceAll(src, "$name$", classname);
	src = core::string::replaceAll(src, "$namespace$", _namespaceSrc);
	src = core::string::replaceAll(src, "$filename$", _shaderDirectory + _shaderStruct.filename);
	std::stringstream uniforms;
	const int uniformSize = int(_shaderStruct.uniforms.size());
	if (uniformSize > 0) {
		uniforms << "checkUniforms({";
		for (int i = 0; i < uniformSize; ++i) {
			std::string uniformName = _shaderStruct.uniforms[i].name;
			uniforms << "\"";
			uniforms << uniformName;
			if (_shaderStruct.uniforms[i].arraySize == -1 || _shaderStruct.uniforms[i].arraySize > 1) {
				uniforms << "[0]";
			}
			uniforms << "\"";
			if (i < uniformSize - 1) {
				uniforms << ", ";
			}
		}
		uniforms << "});";
	} else {
		uniforms << "// no uniforms";
	}
	src = core::string::replaceAll(src, "$uniforms$", uniforms.str());

	std::stringstream attributes;
	const int attributeSize = int(_shaderStruct.attributes.size());
	if (attributeSize > 0) {
		attributes << "checkAttributes({";
		for (int i = 0; i < attributeSize; ++i) {
			const Variable& v = _shaderStruct.attributes[i];
			attributes << "\"" << v.name << "\"";
			if (i < attributeSize - 1) {
				attributes << ", ";
			}
		}
		attributes << "});";
	} else {
		attributes << "// no attributes";
	}

	std::stringstream setters;
	if (uniformSize > 0) {
		setters << "\n";
	}
	for (int i = 0; i < uniformSize; ++i) {
		const Variable& v = _shaderStruct.uniforms[i];
		std::string uniformName = v.name;
		std::vector<std::string> nameParts;
		core::string::splitString(uniformName, nameParts, "_");
		uniformName = "";
		for (std::string n : nameParts) {
			if (n.length() > 1 || nameParts.size() < 2) {
				n[0] = SDL_toupper(n[0]);
				uniformName += n;
			}
		}
		if (uniformName.empty()) {
			uniformName = v.name;
		}
		setters << "\tinline bool set" << uniformName << "(";
		const Types& cType = cTypes[v.type];
		setters << cType.ctype;
		if (v.arraySize == -1 || cType.passBy == PassBy::Pointer) {
			setters << "*";
		} else if (cType.passBy == PassBy::Reference){
			setters << "&";
		} else if (cType.passBy == PassBy::Value){
		}

		setters << " " << v.name;
		if (v.arraySize > 0) {
			setters << "[" << v.arraySize << "]";
		} else if (v.arraySize == -1) {
			setters << ", int amount";
		}
		setters << ") const {\n";

		setters << "\t\tif (!hasUniform(\"" << v.name << "\")) {\n";
		setters << "\t\t\treturn false;\n";
		setters << "\t\t}\n";
		setters << "\t\tsetUniform" << uniformSetterPostfix(v.type, v.arraySize == -1 ? 2 : v.arraySize);
		setters << "(\"" << v.name << "\", " << v.name;
		if (v.arraySize > 0) {
			setters << ", " << v.arraySize;
		} else if (v.arraySize == -1) {
			setters << ", amount";
		}
		setters << ");\n";
		setters << "\t\treturn true;\n";
		setters << "\t}\n";
		if (i < uniformSize- - 2) {
			setters << "\n";
		}
	}

	src = core::string::replaceAll(src, "$attributes$", attributes.str());
	src = core::string::replaceAll(src, "$setters$", setters.str());
	const std::string targetFile = _sourceDirectory + filename;
	Log::info("Generate shader bindings for %s at %s", _shaderStruct.name.c_str(), targetFile.c_str());
	core::App::getInstance()->filesystem()->syswrite(targetFile, src);
}

bool ShaderTool::parse(const std::string& buffer, bool vertex) {
	core::Tokenizer tok(buffer);
	while (tok.hasNext()) {
		const std::string token = tok.next();
		std::vector<Variable>* v = nullptr;
		if (token == "$in") {
			v = &_shaderStruct.attributes;
		} else if (token == "$out") {
			if (vertex) {
				v = &_shaderStruct.varyings;
			} else {
				v = &_shaderStruct.outs;
			}
		} else if (token == "uniform") {
			v = &_shaderStruct.uniforms;
		} else {
			continue;
		}
		if (!tok.hasNext()) {
			Log::error("Failed to parse the shader, could not get type");
			return false;
		}
		std::string type = tok.next();
		if (!tok.hasNext()) {
			Log::error("Failed to parse the shader, could not get variable name for type %s", type.c_str());
			return false;
		}
		while (type == "highp" || type == "mediump" || type == "lowp" || type == "precision") {
			if (!tok.hasNext()) {
				Log::error("Failed to parse the shader, could not get type");
				return false;
			}
			type = tok.next();
		}
		std::string name = tok.next();
		const Variable::Type typeEnum = getType(type);
		bool isArray = false;
		std::string number;
		for (char c : name) {
			if (c == ']') {
				break;
			}
			if (isArray) {
				number.push_back(c);
			}
			if (c == '[') {
				isArray = true;
			}
		}
		int arraySize = core::string::toInt(number);
		if (isArray && arraySize == 0) {
			arraySize = -1;
			Log::warn("Could not determine array size for %s (%s)", name.c_str(), number.c_str());
		}
		if (isArray) {
			std::vector<std::string> tokens;
			core::string::splitString(name, tokens, "[");
			if (tokens.size() != 2) {
				Log::error("Could not extract variable name from %s", name.c_str());
			} else {
				name = tokens[0];
			}
		}
		v->push_back(Variable{typeEnum, name, arraySize});
	}
	return true;
}

core::AppState ShaderTool::onRunning() {
	if (_argc < 3) {
		_exitCode = 1;
		Log::error("Usage: %s <path/to/glslangvalidator> <shaderfile> <namespace> <shader-dir> <src-generator-dir>", _argv[0]);
		return core::AppState::Cleanup;
	}

	for (int i = 0; i < _argc; ++i) {
		Log::debug("argv[%i] = %s", i, _argv[i]);
	}
	const std::string glslangValidatorBin = _argv[1];
	const std::string shaderfile          = _argv[2];
	_namespaceSrc    = _argc >= 4 ?         _argv[3] : "frontend";
	_shaderDirectory = _argc >= 5 ?         _argv[4] : "shaders/";
	_sourceDirectory = _argc >= 6 ?         _argv[5] : _filesystem->basePath() + "src/modules/" + _namespaceSrc + "/";

	Log::debug("Using glslangvalidator binary: %s", glslangValidatorBin.c_str());
	Log::debug("Using %s as output directory", _sourceDirectory.c_str());
	Log::debug("Using %s as namespace", _namespaceSrc.c_str());
	Log::debug("Using %s as shader directory", _shaderDirectory.c_str());

	Log::debug("Preparing shader file %s", shaderfile.c_str());
	const std::string fragmentFilename = shaderfile + FRAGMENT_POSTFIX;
	const std::string fragmentBuffer = filesystem()->load(fragmentFilename);
	if (fragmentBuffer.empty()) {
		Log::error("Could not load %s", fragmentFilename.c_str());
		_exitCode = 1;
		return core::AppState::Cleanup;
	}

	const std::string vertexFilename = shaderfile + VERTEX_POSTFIX;
	const std::string vertexBuffer = filesystem()->load(vertexFilename);
	if (vertexBuffer.empty()) {
		Log::error("Could not load %s", vertexFilename.c_str());
		_exitCode = 1;
		return core::AppState::Cleanup;
	}

	video::Shader shader;
	const std::string& fragmentSrcSource = shader.getSource(video::ShaderType::SHADER_FRAGMENT, fragmentBuffer, false);
	const std::string& vertexSrcSource = shader.getSource(video::ShaderType::SHADER_VERTEX, vertexBuffer, false);

	_shaderStruct.filename = shaderfile;
	_shaderStruct.name = shaderfile;
	parse(fragmentSrcSource, false);
	parse(vertexSrcSource, true);
	generateSrc();

	const std::string& fragmentSource = shader.getSource(video::ShaderType::SHADER_FRAGMENT, fragmentBuffer, true);
	const std::string& vertexSource = shader.getSource(video::ShaderType::SHADER_VERTEX, vertexBuffer, true);

	Log::debug("Writing shader file %s to %s", shaderfile.c_str(), filesystem()->homePath().c_str());
	std::string finalFragmentFilename = _appname + "-" + fragmentFilename;
	std::string finalVertexFilename = _appname + "-" + vertexFilename;
	filesystem()->write(finalFragmentFilename, fragmentSource);
	filesystem()->write(finalVertexFilename, vertexSource);

	Log::debug("Validating shader file %s", shaderfile.c_str());

	std::vector<std::string> fragmentArgs;
	fragmentArgs.push_back(filesystem()->homePath() + finalFragmentFilename);
	int fragmentValidationExitCode = core::Process::exec(glslangValidatorBin, fragmentArgs);

	std::vector<std::string> vertexArgs;
	vertexArgs.push_back(filesystem()->homePath() + finalVertexFilename);
	int vertexValidationExitCode = core::Process::exec(glslangValidatorBin, vertexArgs);

	if (fragmentValidationExitCode != 0) {
		_exitCode = fragmentValidationExitCode;
	} else if (vertexValidationExitCode != 0) {
		_exitCode = vertexValidationExitCode;
	}

	return core::AppState::Cleanup;
}

int main(int argc, char *argv[]) {
	return getInjector()->get<ShaderTool>()->startMainLoop(argc, argv);
}
