/**
 * @file
 */

#include "ShaderTool.h"
#include "io/Filesystem.h"
#include "core/Process.h"
#include "core/String.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/Assert.h"
#include "core/GameConfig.h"
#include "video/Shader.h"
#include "Generator.h"
#include "Parser.h"

ShaderTool::ShaderTool(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "shadertool");
}

bool ShaderTool::parse(const std::string& buffer, bool vertex) {
	return shadertool::parse(_shaderStruct, _shaderfile, buffer, vertex);
}

core::AppState ShaderTool::onConstruct() {
	registerArg("--glslang").setShort("-g").setDescription("Path to glslangvalidator binary").setMandatory();
	registerArg("--shader").setShort("-s").setDescription("The base name of the shader to create the c++ bindings for").setMandatory();
	registerArg("--shadertemplate").setShort("-t").setDescription("The shader template file").setMandatory();
	registerArg("--buffertemplate").setShort("-b").setDescription("The uniform buffer template file").setMandatory();
	registerArg("--namespace").setShort("-n").setDescription("Namespace to generate the source in").setDefaultValue("shader");
	registerArg("--shaderdir").setShort("-d").setDescription("Directory to load the shader from").setDefaultValue("shaders/");
	registerArg("--sourcedir").setDescription("Directory to generate the source in").setMandatory();
	registerArg("--printincludes").setDescription("Print the includes for the given shader");
	Log::trace("Set some shader config vars to let the validation work");
	core::Var::get(cfg::ClientGamma, "2.2", core::CV_SHADER);
	core::Var::get(cfg::ClientFog, "true", core::CV_SHADER);
	core::Var::get(cfg::ClientShadowMap, "true", core::CV_SHADER);
	core::Var::get(cfg::ClientDebugShadow, "false", core::CV_SHADER);
	core::Var::get(cfg::ClientDebugShadowMapCascade, "false", core::CV_SHADER);
	return Super::onConstruct();
}

void ShaderTool::validate(const std::string& name) {
	const std::string& writePath = filesystem()->homePath();
	std::vector<std::string> args;
	args.push_back(writePath + name);
	char output[4096] = "";
	int exitCode = core::Process::exec(_glslangValidatorBin, args, nullptr, sizeof(output), output);
	if (exitCode != 0) {
		Log::error("Failed to validate shader '%s'", name.c_str());
		if (output[0] != '\0') {
			Log::error("%s", output);
		}
		Log::debug("%s %s%s", _glslangValidatorBin.c_str(), writePath.c_str(), name.c_str());
		_exitCode = exitCode;
	}
}

core::AppState ShaderTool::onRunning() {
	const std::string shaderfile          = getArgVal("--shader");
	const bool printIncludes              = hasArg("--printincludes");
	if (!printIncludes) {
		_glslangValidatorBin              = getArgVal("--glslang");
		_shaderTemplateFile               = getArgVal("--shadertemplate");
		_uniformBufferTemplateFile        = getArgVal("--buffertemplate");
		_namespaceSrc                     = getArgVal("--namespace");
		_shaderDirectory                  = getArgVal("--shaderdir");
		_sourceDirectory                  = getArgVal("--sourcedir",
				_filesystem->basePath() + "src/modules/" + _namespaceSrc + "/");

		if (!core::string::endsWith(_shaderDirectory, "/")) {
			_shaderDirectory = _shaderDirectory + "/";
		}
		Log::debug("Using glslangvalidator binary: %s", _glslangValidatorBin.c_str());
		Log::debug("Using %s as output directory", _sourceDirectory.c_str());
		Log::debug("Using %s as namespace", _namespaceSrc.c_str());
		Log::debug("Using %s as shader directory", _shaderDirectory.c_str());
	}

	Log::debug("Preparing shader file %s", shaderfile.c_str());
	_shaderfile = std::string(core::string::extractFilename(shaderfile.c_str()));
	Log::debug("Preparing shader file %s", _shaderfile.c_str());
	const std::string fragmentFilename = _shaderfile + FRAGMENT_POSTFIX;
	const io::FilesystemPtr& fs = filesystem();
	const bool changedDir = fs->pushDir(std::string(core::string::extractPath(shaderfile.c_str())));
	const std::string fragmentBuffer = fs->load(fragmentFilename);
	if (fragmentBuffer.empty()) {
		Log::error("Could not load %s", fragmentFilename.c_str());
		_exitCode = 127;
		return core::AppState::Cleanup;
	}

	const std::string vertexFilename = _shaderfile + VERTEX_POSTFIX;
	const std::string vertexBuffer = fs->load(vertexFilename);
	if (vertexBuffer.empty()) {
		Log::error("Could not load %s", vertexFilename.c_str());
		_exitCode = 127;
		return core::AppState::Cleanup;
	}

	const std::string geometryFilename = _shaderfile + GEOMETRY_POSTFIX;
	const std::string geometryBuffer = fs->load(geometryFilename);

	const std::string computeFilename = _shaderfile + COMPUTE_POSTFIX;
	const std::string computeBuffer = fs->load(computeFilename);

	video::Shader shader;

	std::vector<std::string> includes;
	const std::string& fragmentSrcSource = shader.getSource(video::ShaderType::Fragment, fragmentBuffer, false, &includes);
	const std::string& vertexSrcSource = shader.getSource(video::ShaderType::Vertex, vertexBuffer, false, &includes);

	_shaderStruct.filename = _shaderfile;
	_shaderStruct.name = _shaderfile;
	if (!parse(fragmentSrcSource, false)) {
		Log::error("Failed to parse fragment shader %s", _shaderfile.c_str());
		_exitCode = 1;
		return core::AppState::Cleanup;
	}
	if (!parse(vertexSrcSource, true)) {
		Log::error("Failed to parse vertex shader %s", _shaderfile.c_str());
		_exitCode = 1;
		return core::AppState::Cleanup;
	}
	if (!geometryBuffer.empty()) {
		const std::string& geometrySrcSource = shader.getSource(video::ShaderType::Geometry, geometryBuffer, false, &includes);
		if (!parse(geometrySrcSource, false)) {
			Log::error("Failed to parse geometry shader %s", _shaderfile.c_str());
			_exitCode = 1;
			return core::AppState::Cleanup;
		}
	}
	if (!computeBuffer.empty()) {
		const std::string& computeSrcSource = shader.getSource(video::ShaderType::Compute, computeBuffer, false, &includes);
		if (!parse(computeSrcSource, false)) {
			Log::error("Failed to parse compute shader %s", _shaderfile.c_str());
			_exitCode = 1;
			return core::AppState::Cleanup;
		}
	}

	if (printIncludes) {
		const std::string& path = std::string(core::string::extractPath(shaderfile.c_str()));
		for (const std::string& i : includes) {
			Log::info("%s%s", path.c_str(), i.c_str());
		}
		return core::AppState::Cleanup;
	}

	for (const auto& block : _shaderStruct.uniformBlocks) {
		Log::debug("Found uniform block %s with %i members", block.name.c_str(), int(block.members.size()));
	}
	for (const auto& v : _shaderStruct.uniforms) {
		Log::debug("Found uniform of type %i with name %s", int(v.type), v.name.c_str());
	}
	for (const auto& v : _shaderStruct.attributes) {
		Log::debug("Found attribute of type %i with name %s", int(v.type), v.name.c_str());
	}
	for (const auto& v : _shaderStruct.varyings) {
		Log::debug("Found varying of type %i with name %s", int(v.type), v.name.c_str());
	}
	for (const auto& v : _shaderStruct.outs) {
		Log::debug("Found out var of type %i with name %s", int(v.type), v.name.c_str());
	}

	const std::string& templateShader = fs->load(_shaderTemplateFile);
	const std::string& templateUniformBuffer = fs->load(_uniformBufferTemplateFile);
	if (!shadertool::generateSrc(templateShader, templateUniformBuffer, _shaderStruct,
			filesystem(), _namespaceSrc, _sourceDirectory, _shaderDirectory)) {
		Log::error("Failed to generate shader source for %s", _shaderfile.c_str());
		_exitCode = 1;
		return core::AppState::Cleanup;
	}

	const std::string& fragmentSource = shader.getSource(video::ShaderType::Fragment, fragmentBuffer, true);
	const std::string& vertexSource = shader.getSource(video::ShaderType::Vertex, vertexBuffer, true);
	const std::string& geometrySource = shader.getSource(video::ShaderType::Geometry, geometryBuffer, true);
	const std::string& computeSource = shader.getSource(video::ShaderType::Compute, computeBuffer, true);

	if (changedDir) {
		fs->popDir();
	}

	const std::string& writePath = fs->homePath();
	Log::debug("Writing shader file %s to %s", _shaderfile.c_str(), writePath.c_str());
	std::string finalFragmentFilename = _appname + "-" + fragmentFilename;
	std::string finalVertexFilename = _appname + "-" + vertexFilename;
	std::string finalGeometryFilename = _appname + "-" + geometryFilename;
	std::string finalComputeFilename = _appname + "-" + computeFilename;
	fs->write(finalFragmentFilename, fragmentSource);
	fs->write(finalVertexFilename, vertexSource);
	if (!geometrySource.empty()) {
		fs->write(finalGeometryFilename, geometrySource);
	}
	if (!computeSource.empty()) {
		fs->write(finalComputeFilename, computeSource);
	}

	Log::debug("Validating shader file %s", _shaderfile.c_str());

	validate(finalFragmentFilename);
	validate(finalVertexFilename);
	if (!geometrySource.empty()) {
		validate(finalGeometryFilename);
	}
	if (!computeSource.empty()) {
		validate(finalComputeFilename);
	}

	return core::AppState::Cleanup;
}

CONSOLE_APP(ShaderTool)
