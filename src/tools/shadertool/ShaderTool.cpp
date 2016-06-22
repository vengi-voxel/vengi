/**
 * @file
 */

#include "ShaderTool.h"
#include "sauce/ShaderToolInjector.h"
#include "core/App.h"
#include "core/Process.h"
#include "core/Tokenizer.h"
#include "video/Shader.h"

ShaderTool::ShaderTool(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		core::App(filesystem, eventBus, 0) {
	init("engine", "shadertool");
}

ShaderTool::~ShaderTool() {
}

ShaderTool::Variable::Type ShaderTool::getType(const std::string& type) const {
	if (type == "float") {
		return Variable::FLOAT;
	} else if (type == "bool") {
		return Variable::BOOL;
	} else if (type == "int") {
		return Variable::INT;
	} else if (type == "vec2") {
		return Variable::VEC2;
	} else if (type == "vec3") {
		return Variable::VEC3;
	} else if (type == "vec4") {
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
		const std::string type = tok.next();
		if (!tok.hasNext()) {
			Log::error("Failed to parse the shader, could not get variable name for type %s", type.c_str());
			return false;
		}
		const std::string name = tok.next();
		const Variable::Type typeEnum = getType(type);
		v->push_back(Variable{typeEnum, name});
	}
	return true;
}

core::AppState ShaderTool::onRunning() {
	if (_argc != 3) {
		_exitCode = 1;
		Log::error("Usage: %s <path/to/glslangvalidator> <shaderfile>", _argv[0]);
		return core::AppState::Cleanup;
	}

	const std::string glslangValidatorBin = _argv[1];
	Log::debug("Using glslangvalidator binary: %s", glslangValidatorBin.c_str());

	const std::string shaderfile = _argv[2];

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
